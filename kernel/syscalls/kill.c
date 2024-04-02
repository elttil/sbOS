#include <errno.h>
#include <sched/scheduler.h>
#include <signal.h>
#include <syscalls.h>

int syscall_kill(int fd, int sig) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd, NULL);
  if (!fd_ptr) {
    return -EBADF;
  }
  if (!fd_ptr->inode->send_signal) {
    return -EBADF;
  }
  return process_signal(fd_ptr, sig);
}
