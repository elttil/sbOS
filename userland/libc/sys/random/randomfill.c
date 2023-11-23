#include <sys/random.h>
#include <syscall.h>

void randomfill(void *buffer, uint32_t size) {
  syscall(SYS_RANDOMFILL, buffer, size, 0, 0, 0);
}
