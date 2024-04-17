#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <syscall.h>

int stat(const char *path, struct stat *buf) {
  int fd = open(path, O_RDONLY);
  if (-1 == fd) {
    return -1;
  }
  int rc = fstat(fd, buf);
  close(fd);
  return rc;
}
