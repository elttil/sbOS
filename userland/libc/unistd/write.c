#include <errno.h>
#include <syscall.h>
#include <unistd.h>

int write(int fd, const char *buf, size_t count) {
  RC_ERRNO(syscall(SYS_WRITE, fd, buf, count, 0, 0));
}
