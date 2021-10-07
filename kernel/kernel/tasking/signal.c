/*
 * Copyright (C) 2020-2021 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <libkern/bits/errno.h>
#include <libkern/libkern.h>
#include <mem/vmm/vmm.h>
#include <mem/vmm/zoner.h>
#include <platform/generic/system.h>
#include <platform/generic/tasking/signal_impl.h>
#include <tasking/sched.h>
#include <tasking/signal.h>
#include <tasking/tasking.h>
#include <tasking/thread.h>

#define MAGIC_STATE_JUST_TF 0xfeed3eee
#define MAGIC_STATE_NEW_STACK 0xea12002a

#ifdef __i386__
#define return_tf (thread->tf->ebx)
#elif __arm__
#define return_tf (thread->tf->r[1])
#endif

enum {
    UNBLOCK,
    SKIP,
};

static zone_t _signal_jumper_zone;

extern void signal_caller_start();
extern void signal_caller_end();

/**
 * INIT
 */

static void _signal_init_caller()
{
    _signal_jumper_zone = zoner_new_zone(VMM_PAGE_SIZE);
    vmm_load_page(_signal_jumper_zone.start, PAGE_WRITABLE | PAGE_EXECUTABLE | PAGE_READABLE | PAGE_USER);
    uint32_t signal_caller_len = (uint32_t)signal_caller_end - (uint32_t)signal_caller_start;
    memcpy(_signal_jumper_zone.ptr, (void*)signal_caller_start, signal_caller_len);
    vmm_tune_page(_signal_jumper_zone.start, PAGE_EXECUTABLE | PAGE_READABLE | PAGE_USER);
}

void signal_init()
{
    _signal_init_caller();
}

/**
 * HELPER FUNCTIONS
 */

int signal_set_handler(thread_t* thread, int signo, void* handler)
{
    if (signo < 0 || signo >= SIGNALS_CNT) {
        return -EINVAL;
    }
    thread->signal_handlers[signo] = handler;
    return 0;
}

/**
 * Makes signal allow to be called
 */
int signal_set_allow(thread_t* thread, int signo)
{
    if (signo < 0 || signo >= SIGNALS_CNT) {
        return -EINVAL;
    }
    thread->signals_mask |= (1 << signo);
    return 0;
}

/**
 * Makes signal private. It can't be called.
 */
int signal_set_private(thread_t* thread, int signo)
{
    if (signo < 0 || signo >= SIGNALS_CNT) {
        return -EINVAL;
    }
    thread->signals_mask &= ~((uint32_t)(1 << signo));
    return 0;
}

int signal_set_pending(thread_t* thread, int signo)
{
    if (signo < 0 || signo >= SIGNALS_CNT) {
        return -EINVAL;
    }
    thread->pending_signals_mask |= (1 << signo);
    return 0;
}

int signal_rem_pending(thread_t* thread, int signo)
{
    if (signo < 0 || signo >= SIGNALS_CNT) {
        return -EINVAL;
    }
    thread->pending_signals_mask &= ~((uint32_t)(1 << signo));
    return 0;
}

extern int _thread_setup_kstack(thread_t* thread, uint32_t esp);
static int signal_setup_stack_to_handle_signal(thread_t* thread, int signo)
{
    system_disable_interrupts();
    pdirectory_t* prev_pdir = vmm_get_active_pdir();
    vmm_switch_pdir(thread->process->pdir);
    vmm_prepare_active_pdir_for_copying_at((uint32_t)thread->tf, 1);

    uint32_t old_sp = get_stack_pointer(thread->tf);
    uint32_t magic = MAGIC_STATE_JUST_TF; /* helps to restore thread after sgnal to the right state */

    /* TODO: Add support for SMP */
    if (thread != RUNNING_THREAD) {
        /* 
        If we are here that means that the thread was stopped while
        being in kernel (because of scheduler or blocker). That means,
        we need not corrupt it's kernel state, so we create a new state
        to send signal.

        We setup a new kernel state upper the previous one
        Stack:
            Trapframe
            Function calls and local vars...
            Context
        ---- added after setup ----
            Trapframe for signal
            Context for signal
        */

        magic = MAGIC_STATE_NEW_STACK;
        trapframe_t* old_tf = thread->tf;
        context_t* old_ctx = thread->context;

        _thread_setup_kstack(thread, (uint32_t)thread->context);
        memcpy(thread->tf, old_tf, sizeof(trapframe_t));

        tf_push_to_stack(thread->tf, (uint32_t)old_tf);
        tf_push_to_stack(thread->tf, (uint32_t)old_ctx);
        tf_push_to_stack(thread->tf, (uint32_t)old_tf ^ (uint32_t)old_ctx); // checksum
    }

    signal_impl_prepare_stack(thread, signo, old_sp, magic);
    vmm_switch_pdir(prev_pdir);
    system_enable_interrupts();
    return 0;
}

int signal_restore_thread_after_handling_signal(thread_t* thread)
{
    int ret = return_tf;

    uint32_t old_sp, magic;
    signal_impl_restore_stack(thread, &old_sp, &magic);

    if (magic == MAGIC_STATE_NEW_STACK) {
        uint32_t checksum = tf_pop_to_stack(thread->tf);
        context_t* old_ctx = (context_t*)tf_pop_to_stack(thread->tf);
        trapframe_t* old_tf = (trapframe_t*)tf_pop_to_stack(thread->tf);

        uint32_t calced_checksum = ((uint32_t)old_tf ^ (uint32_t)old_ctx);

        if (checksum != calced_checksum) {
            log_error("Killed %d: wrong signal checksum\n", thread->process->pid);
            proc_die(thread->process);
            resched_dont_save_context();
        }

        thread->tf = old_tf;
        thread->context = old_ctx;
    }

    if (old_sp != get_stack_pointer(thread->tf)) {
        log_error("SPs are diff after signal");
    }

    /* If our thread was blocked, that means that it already has a context on stack, we need not to overwrite it */
    if (thread->blocker.reason != BLOCKER_INVALID) {
        thread->status = THREAD_STATUS_BLOCKED;
        sched_dequeue(thread);
        resched_dont_save_context();
    }

    /* Since we already have a context on stack, we need not to overwrite it */
    if (magic == MAGIC_STATE_NEW_STACK) {
        resched_dont_save_context();
    }

    return ret;
}

static int signal_default_action(int signo)
{
    if (signo == 9) {
        return SIGNAL_ACTION_TERMINATE;
    }
    return -1;
}

/* FIXME: Don't allow to run a signal while other is in process */
static int signal_process(thread_t* thread, int signo)
{
    if (thread->signal_handlers[signo]) {
        signal_setup_stack_to_handle_signal(thread, signo);
        set_instruction_pointer(thread->tf, _signal_jumper_zone.start);
        return UNBLOCK;
    } else {
        int result = signal_default_action(signo);
        if (result == SIGNAL_ACTION_TERMINATE) {
            proc_die(thread->process);
            return SKIP;
        }
    }
    return -EFAULT;
}

int signal_dispatch_pending(thread_t* thread)
{
    uint32_t candidate_signals = thread->pending_signals_mask & thread->signals_mask;

    if (!candidate_signals) {
        return -EACCES;
    }

    uint32_t signo = 1;
    for (; signo < 32; signo++) {
        if (candidate_signals & (1 << signo)) {
            break;
        }
    }

    signal_rem_pending(thread, signo);
    int ret = signal_process(thread, signo);

    if (ret < 0) {
        return ret;
    }

    if (ret == UNBLOCK) {
        if (thread && thread->status == THREAD_STATUS_BLOCKED && thread->blocker.should_unblock_for_signal) {
            sched_enqueue(thread);
        }
    }

    return 0;
}
