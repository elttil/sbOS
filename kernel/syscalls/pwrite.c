#include <fs/vfs.h>
#include <syscalls.h>

int syscall_pwrite(int fd, const char *buf, size_t count, size_t offset) {
  return vfs_pwrite(fd, (char*)buf, count, offset);
}
