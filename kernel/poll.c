#include <errno.h>
#include <fs/vfs.h>
#include <interrupts.h>
#include <lib/list.h>
#include <poll.h>

static int poll_check(struct pollfd *fds, size_t nfds) {
  int rc = 0;
  for (size_t i = 0; i < nfds; i++) {
    fds[i].revents = 0;
    if (0 > fds[i].fd) {
      continue;
    }
    vfs_fd_t *f = get_vfs_fd(fds[i].fd, NULL);
    if (!f) {
      if (fds[i].events & POLLHUP) {
        fds[i].revents |= POLLHUP;
      }
    } else {
      if ((fds[i].events & POLLIN) && f->inode->_has_data) {
        if (f->inode->_has_data(f->inode)) {
          fds[i].revents |= POLLIN;
        }
      }
      if ((fds[i].events & POLLOUT) && f->inode->_can_write) {
        if (f->inode->_can_write(f->inode)) {
          fds[i].revents |= POLLOUT;
        }
      }
      if ((fds[i].events & POLLHUP) && !(f->inode->is_open)) {
        fds[i].revents |= POLLHUP;
      }
    }
    if (fds[i].revents) {
      rc++;
    }
  }
  return rc;
}

int poll(struct pollfd *fds, size_t nfds, int timeout) {
  (void)timeout;
  {
    int rc;
    if ((rc = poll_check(fds, nfds)) > 0) {
      return rc;
    }
  }
  disable_interrupts();

  struct list *read_list = &current_task->read_list;
  struct list *write_list = &current_task->write_list;
  struct list *disconnect_list = &current_task->disconnect_list;
  for (size_t i = 0; i < nfds; i++) {
    if (fds[i].fd < 0) {
      continue;
    }
    vfs_fd_t *f = get_vfs_fd(fds[i].fd, NULL);
    if (NULL == f) {
      continue;
    }

    if (fds[i].events & POLLIN) {
      list_add(read_list, f->inode, NULL);
    }
    if (fds[i].events & POLLOUT) {
      list_add(write_list, f->inode, NULL);
    }
    if (fds[i].events & POLLHUP) {
      list_add(disconnect_list, f->inode, NULL);
    }
  }

  switch_task();
  disable_interrupts();

  list_reset(read_list);
  list_reset(write_list);
  list_reset(disconnect_list);
  if (current_task->is_interrupted) {
    current_task->is_interrupted = 0;
    current_task->is_halted = 0;
    return -EINTR;
  }

  return poll_check(fds, nfds);
}
