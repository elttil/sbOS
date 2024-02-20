#include <errno.h>
#include <fs/vfs.h>
#include <syscalls.h>

int syscall_write(int fd, const char *buf, size_t count) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd);
  if (!fd_ptr) {
    return -EBADF;
  }
  int rc = vfs_pwrite(fd, (char *)buf, count, fd_ptr->offset);
  fd_ptr->offset += rc;
  return rc;
}
