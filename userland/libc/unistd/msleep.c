// Not standard, but it feels like it should be.
#include <syscall.h>
#include <unistd.h>
#include <stdint.h>

void msleep(uint32_t ms) { syscall(SYS_MSLEEP, (void *)ms, 0, 0, 0, 0); }
