#include "log.h"
#include <cpu/arch_inst.h>
#include <drivers/serial.h>
#include <drivers/vbe.h>
#include <fonts.h>
#include <sched/scheduler.h>
#include <stdarg.h>

int log_to_screen = 0;

extern u8 *framebuffer;
extern u32 framebuffer_width;
extern u32 framebuffer_height;

void log_enable_screen(void) {
  log_to_screen = 1;
  u32 *p = (u32 *)framebuffer;
  for (u32 i = 0; i < framebuffer_width * framebuffer_height; i++, p++) {
    *p = 0xFF0000;
  }
}

struct stackframe {
  struct stackframe *ebp;
  u32 eip;
};

u32 x = 0;
u32 y = 0;

void log_char(const char c) {
  write_serial(c);
  if (log_to_screen) {
    if ('\n' == c) {
      x = 0;
      y += 8;
      return;
    }
    if (x > 1000) {
      y += 8;
      x = 0;
    }
    vbe_drawfont(x, y, c);
    x += 8;
  }
}

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
