#include <drivers/pit.h>
#include <scalls/msleep.h>
#include <sched/scheduler.h>
#include <stdio.h>

void syscall_msleep(uint32_t ms) {
  get_current_task()->sleep_until = pit_num_ms() + ms;
  switch_task();
}
