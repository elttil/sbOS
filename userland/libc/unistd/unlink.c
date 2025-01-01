#include <stdio.h>
#include <syscall.h>
#include <unistd.h>

int unlink(const char *path) {
  RC_ERRNO(syscall(SYS_UNLINK, path, 0, 0, 0, 0));
}
