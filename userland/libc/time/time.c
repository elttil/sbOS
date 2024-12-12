#include <time.h>

time_t time(time_t *tloc) {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  if (tloc) {
    *tloc = ts.tv_sec;
  }
  return ts.tv_sec;
}
