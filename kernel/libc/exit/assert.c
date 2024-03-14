#include <assert.h>
#include <log.h>
#include <stdio.h>
#include <cpu/arch_inst.h>

__attribute__((__noreturn__)) void aFailed(char *f, int l) {
  kprintf("Assert failed\n");
  kprintf("%s : %d\n", f, l);
  dump_backtrace(10);
  halt();
  kprintf("after halt?\n");
}
