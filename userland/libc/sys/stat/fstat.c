#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <syscall.h>

int fstat(int fd, struct stat *buf) {
  RC_ERRNO(syscall(SYS_FSTAT, fd, buf, 0, 0, 0));
}
