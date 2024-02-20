#include <errno.h>
#include <fs/vfs.h>
#include <syscalls.h>

int syscall_isatty(int fd) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd);
  if (!fd_ptr) {
    return -EBADF;
  }
  if (!fd_ptr->is_tty) {
    return -ENOTTY;
  }
  return 1;
}
