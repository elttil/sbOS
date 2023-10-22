#include "log.h"
#include <sched/scheduler.h>

struct stackframe {
  struct stackframe *ebp;
  uint32_t eip;
};

void dump_backtrace(uint32_t max_frames) {
  struct stackframe *stk;
  asm("mov %%ebp,%0" : "=r"(stk)::);
  kprintf("Stack trace:\n");
  for (uint32_t frame = 0; stk && frame < max_frames; ++frame) {
    kprintf(" 0x%x\n", stk->eip);
    stk = stk->ebp;
  }
  if (get_current_task()) {
    kprintf(" PID: %x\n", get_current_task()->pid);
  }
}

void klog(char *str, int code) {
  switch (code) {
  case LOG_NOTE:
    kprintf("[NOTE] ");
    break;
  case LOG_WARN:
    kprintf("[WARN] ");
    break;
  case LOG_ERROR:
    kprintf("[ERROR] ");
    break;
  default:
  case LOG_SUCCESS:
    kprintf("[SUCCESS] ");
    break;
  }

  puts(str);
}
