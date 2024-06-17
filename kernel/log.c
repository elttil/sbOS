#include "log.h"
#include <cpu/arch_inst.h>
#include <sched/scheduler.h>
#include <stdarg.h>

struct stackframe {
  struct stackframe *ebp;
  u32 eip;
};

void dump_backtrace(u32 max_frames) {
  struct stackframe *stk = (void *)get_current_sbp();
  kprintf("Stack trace:\n");
  for (u32 frame = 0; stk && frame < max_frames; ++frame) {
    kprintf(" 0x%x\n", stk->eip);
    stk = stk->ebp;
  }
  if (current_task) {
    kprintf(" PID: %x\n", current_task->pid);
  }
}

void klog(int code, char *fmt, ...) {
  va_list list;
  va_start(list, fmt);
  switch (code) {
  case LOG_SUCCESS:
    kprintf("[SUCCESS] ");
    break;
  case LOG_NOTE:
    kprintf("[NOTE] ");
    break;
  case LOG_WARN:
    kprintf("[WARN] ");
    break;
  default:
  case LOG_ERROR:
    kprintf("[ERROR] ");
    break;
  }
  vkprintf(fmt, list);
  va_end(list);
  kprintf("\n");
}
