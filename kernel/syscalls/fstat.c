#include <errno.h>
#include <fs/vfs.h>
#include <syscalls.h>

int syscall_fstat(int fd, struct stat *buf) {
  if (!mmu_is_valid_userpointer(buf, sizeof(struct stat))) {
    return -EPERM; // TODO: Is this correct? The spec says nothing about
                   // this case.
  }
  return vfs_fstat(fd, buf);
}
