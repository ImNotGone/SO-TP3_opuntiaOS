/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _KERNEL_PLATFORM_AARCH32_TASKING_SIGNAL_IMPL_H
#define _KERNEL_PLATFORM_AARCH32_TASKING_SIGNAL_IMPL_H

#include <libkern/types.h>
struct thread;

int signal_impl_prepare_stack(struct thread* thread, int signo, uint32_t old_sp, uint32_t magic);
int signal_impl_restore_stack(struct thread* thread, uint32_t* old_sp, uint32_t* magic);

#endif // _KERNEL_PLATFORM_AARCH32_TASKING_SIGNAL_IMPL_H