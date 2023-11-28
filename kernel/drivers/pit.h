#ifndef PIT_H
#define PIT_H
#include <cpu/idt.h>
#include <cpu/io.h>
#include <sched/scheduler.h>
#include <stdio.h>
#include <typedefs.h>

void pit_install(void);
void set_pit_count(u16 hertz);
u64 pit_num_ms(void);
#endif
