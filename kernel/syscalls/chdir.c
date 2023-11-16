#include <fs/vfs.h>
#include <syscalls.h>

int syscall_chdir(const char *path) {
  return vfs_chdir(path);
}
