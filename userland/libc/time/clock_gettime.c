#include <syscall.h>
#include <time.h>

int clock_gettime(clockid_t clock_id, struct timespec *tp) {
	(void)clock_id;
  tp->tv_sec = 0;
  tp->tv_nsec = 0;
  return 0;
  /*
SYS_CLOCK_GETTIME_PARAMS args = {
.clk = clock_id,
.ts = tp,
};
return syscall(SYS_CLOCK_GETTIME, &args);*/
}
