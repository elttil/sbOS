// Not standard, but it feels like it should be.
#include <stdint.h>
#include <syscall.h>
#include <unistd.h>

uint32_t uptime(void) { return syscall(SYS_UPTIME, 0, 0, 0, 0, 0); }
