#ifndef TIME_H
#define TIME_H
#include <types.h>

struct timespec {
  time_t tv_sec; // Seconds.
  long tv_nsec;  // Nanoseconds.
};
#endif
