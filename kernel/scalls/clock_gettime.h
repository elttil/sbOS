#include <time.h>

typedef struct SYS_CLOCK_GETTIME_PARAMS {
    clockid_t clk;
    struct timespec *ts;
} __attribute__((packed)) SYS_CLOCK_GETTIME_PARAMS;

int syscall_clock_gettime(SYS_CLOCK_GETTIME_PARAMS *args);
