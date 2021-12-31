/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <drivers/aarch32/pl031.h>
#include <fs/devfs/devfs.h>
#include <fs/vfs.h>
#include <libkern/bits/errno.h>
#include <libkern/libkern.h>
#include <libkern/log.h>
#include <mem/kmemzone.h>
#include <mem/vmm.h>
#include <tasking/tasking.h>

// #define DEBUG_PL031

static kmemzone_t mapped_zone;
static volatile pl031_registers_t* registers = (pl031_registers_t*)PL031_BASE;

static inline int _pl031_map_itself()
{
    mapped_zone = kmemzone_new(sizeof(pl031_registers_t));
    vmm_map_page(mapped_zone.start, PL031_BASE, PAGE_READABLE | PAGE_WRITABLE | PAGE_EXECUTABLE);
    registers = (pl031_registers_t*)mapped_zone.ptr;
    return 0;
}

void pl031_install()
{
    if (_pl031_map_itself()) {
#ifdef DEBUG_PL031
        log_error("PL031: Can't map itself!");
#endif
        return;
    }
}

uint32_t pl031_read_rtc()
{
    return registers->data;
}
