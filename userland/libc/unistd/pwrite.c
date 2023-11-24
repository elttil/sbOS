#include <syscall.h>
#include <unistd.h>

int pwrite(int fd, const char *buf, size_t count, size_t offset) {
  return syscall(SYS_PWRITE, fd, buf, count, offset, 0);
}
