#include <syscalls.h>

int syscall_mkdir(const char *path, int mode) {
  return vfs_mkdir(path, mode);
}
