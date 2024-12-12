// Not standard, but it feels like it should be.
#include <stdint.h>
#include <syscall.h>
#include <unistd.h>

void msleep(uint32_t ms) {
  syscall(SYS_MSLEEP, (void *)ms, 0, 0, 0, 0);
}
