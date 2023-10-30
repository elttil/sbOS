#include <types.h>

struct timespec {
  time_t tv_sec; // Seconds.
  long tv_nsec;  // Nanoseconds.
};
