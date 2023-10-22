#ifndef PIT_H
#define PIT_H
#include <cpu/idt.h>
#include <cpu/io.h>
#include <sched/scheduler.h>
#include <stdint.h>
#include <stdio.h>

void pit_install(void);
void set_pit_count(uint16_t hertz);
uint64_t pit_num_ms(void);
#endif
