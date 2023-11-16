#include <syscalls.h>

int syscall_clock_gettime(SYS_CLOCK_GETTIME_PARAMS *args) {
  // FIXME: Actually implement this
  if (args->ts) {
    args->ts->tv_sec = 0;
    args->ts->tv_nsec = 0;
  }
  return 0;
}
