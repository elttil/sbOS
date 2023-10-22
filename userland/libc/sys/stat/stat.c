#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <syscall.h>

int stat(const char *path, struct stat *buf) {
  SYS_STAT_PARAMS args = {
      .pathname = path,
      .statbuf = buf,
  };
  RC_ERRNO(syscall(SYS_STAT, &args, 0, 0, 0, 0));
}
