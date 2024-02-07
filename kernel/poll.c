#include <fs/vfs.h>
#include <halts.h>
#include <poll.h>
#include <sched/scheduler.h>

int poll(struct pollfd *fds, size_t nfds, int timeout) {
  (void)timeout;
  int rc = 0;
  int read_locks[nfds];
  int write_locks[nfds];
  int disconnect_locks[nfds];
  for (size_t i = 0; i < nfds; i++) {
    if (fds[i].fd < 0)
      continue;
    vfs_fd_t *f = get_vfs_fd(fds[i].fd);
    if (fds[i].events & POLLIN)
      read_locks[i] = create_read_fdhalt(f);
    if (fds[i].events & POLLOUT)
      write_locks[i] = create_write_fdhalt(f);
    if (fds[i].events & POLLHUP)
      disconnect_locks[i] = create_disconnect_fdhalt(f);
  }

  switch_task(1);

  for (size_t i = 0; i < nfds; i++) {
    if (fds[i].fd < 0)
      continue;
    if (fds[i].events & POLLIN)
      unset_read_fdhalt(read_locks[i]);
    if (fds[i].events & POLLOUT)
      unset_write_fdhalt(write_locks[i]);
    if (fds[i].events & POLLHUP)
      unset_disconnect_fdhalt(disconnect_locks[i]);
  }
  for (size_t i = 0; i < nfds; i++) {
    if (0 > fds[i].fd) {
      fds[i].revents = 0;
      continue;
    }
    vfs_fd_t *f = get_vfs_fd(fds[i].fd);
    if (!f) {
      if (fds[i].events & POLLHUP)
        fds[i].revents |= POLLHUP;
    } else {
      if (f->inode->has_data && fds[i].events & POLLIN)
        fds[i].revents |= POLLIN;
      if (f->inode->can_write && fds[i].events & POLLOUT)
        fds[i].revents |= POLLOUT;
      if (!(f->inode->is_open) && fds[i].events & POLLHUP)
        fds[i].revents |= POLLHUP;
      if (fds[i].revents)
        rc++;
    }
  }
  return rc;
}
