typedef int clockid_t;
#ifndef TIME_H
#define TIME_H
#include <sys/types.h>

struct timespec {
  time_t tv_sec; // Seconds.
  long tv_nsec;  // Nanoseconds.
};
#endif
