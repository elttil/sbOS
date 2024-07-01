#include <errno.h>
#include <stdint.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>

int clock_gettime(clockid_t clock_id, struct timespec *tp) {
  uint32_t ms = uptime();
  tp->tv_sec = ms / 1000;
  tp->tv_nsec = ms * 1000000;
  RC_ERRNO(syscall(SYS_CLOCK_GETTIME, clock_id, tp, 0, 0, 0));
}
