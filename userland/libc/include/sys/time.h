#ifndef TIME_H
#define TIME_H
#include <stddef.h>
#include <sys/types.h>
#include <time.h>

#define CLOCK_REALTIME 0

struct tm {
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
  long __tm_gmtoff;
  const char *__tm_zone;
};

typedef int clockid_t;
struct timespec {
  time_t tv_sec; // Seconds.
  long tv_nsec;  // Nanoseconds.
};

struct timeval {
  time_t tv_sec;
  suseconds_t tv_usec;
};

time_t time(time_t *tloc);
int clock_gettime(clockid_t clock_id, struct timespec *tp);
struct tm *localtime(const time_t *timer);
struct tm *gmtime(const time_t *timer);
size_t strftime(char *restrict s, size_t maxsize, const char *restrict format,
                const struct tm *restrict timeptr);
int gettimeofday(struct timeval *tp, void *tzp);
#endif
