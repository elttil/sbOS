#include <drivers/pit.h>
#include <sched/scheduler.h>
#include <stdio.h>
#include <syscalls.h>

void syscall_msleep(u32 ms) {
  get_current_task()->sleep_until = pit_num_ms() + ms;
  switch_task(1);
}
