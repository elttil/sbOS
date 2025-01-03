#include <stdio.h>
#include <syscall.h>
#include <unistd.h>

int chroot(const char *path) {
  RC_ERRNO(syscall(SYS_CHROOT, path, 0, 0, 0, 0));
}
