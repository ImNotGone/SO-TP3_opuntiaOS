/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _KERNEL_PLATFORM_AARCH32_INTERRUPTS_H
#define _KERNEL_PLATFORM_AARCH32_INTERRUPTS_H

#include <libkern/mask.h>
#include <libkern/types.h>

#define IRQ_HANDLERS_MAX 256
#define ALL_CPU_MASK 0xff
#define BOOT_CPU_MASK 0x01

typedef int irq_type_t;
typedef int irq_line_t;
typedef uint8_t irq_priority_t;
typedef void (*irq_handler_t)();

enum IRQTypeMasks {
    MASKDEFINE(IRQ_TYPE_EDGE_TRIGGERED, 0, 1),
};

struct gic_descritptor {
    uint32_t (*interrupt_descriptor)();
    void (*end_interrupt)(uint32_t int_desc);
    void (*enable_irq)(irq_line_t line, irq_priority_t prior, irq_type_t type, int cpu_mask);
};
typedef struct gic_descritptor gic_descritptor_t;

void interrupts_setup();
void interrupts_setup_secondary_cpu();

extern char STACK_ABORT_TOP;
extern char STACK_UNDEFINED_TOP;
extern char STACK_IRQ_TOP;
extern char STACK_SVC_TOP;
extern char STACK_TOP;

extern void swi(uint32_t num);
extern void set_svc_stack(uint32_t stack);
extern void set_irq_stack(uint32_t stack);
extern void set_abort_stack(uint32_t stack);
extern void set_undefined_stack(uint32_t stack);

extern void reset_handler();
extern void undefined_handler();
extern void svc_handler();
extern void prefetch_abort_handler();
extern void data_abort_handler();
extern void irq_handler();
extern void fast_irq_handler();

void irq_register_handler(irq_line_t line, irq_priority_t prior, irq_type_t type, irq_handler_t func, int cpu_mask);
void irq_set_gic_desc(gic_descritptor_t gic_desc);

void gic_setup();
void gic_setup_secondary_cpu();

#endif /* _KERNEL_PLATFORM_AARCH32_INTERRUPTS_H */