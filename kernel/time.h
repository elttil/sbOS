#ifndef TIME_H
#define TIME_H
#include <sys/types.h>

typedef int clockid_t;
struct timespec {
  time_t tv_sec; // Seconds.
  long tv_nsec;  // Nanoseconds.
};
#endif
