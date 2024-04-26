#include <errno.h>
#include <fs/vfs.h>
#include <interrupts.h>
#include <lib/list.h>
#include <poll.h>
#include <sched/scheduler.h>

int poll(struct pollfd *fds, size_t nfds, int timeout) {
  (void)timeout;
  int rc = 0;

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

  for (size_t i = 0; i < nfds; i++) {
    if (0 > fds[i].fd) {
      fds[i].revents = 0;
      continue;
    }
    vfs_fd_t *f = get_vfs_fd(fds[i].fd, NULL);
    if (!f) {
      if (fds[i].events & POLLHUP) {
        fds[i].revents |= POLLHUP;
      }
    } else {
      if (f->inode->_has_data) {
        if (f->inode->_has_data(f->inode) && fds[i].events & POLLIN) {
          fds[i].revents |= POLLIN;
        }
      }
      if (f->inode->_can_write) {
        if (f->inode->_can_write(f->inode) && fds[i].events & POLLOUT) {
          fds[i].revents |= POLLOUT;
        }
      }
      if (!(f->inode->is_open) && fds[i].events & POLLHUP) {
        fds[i].revents |= POLLHUP;
      }
    }
    if (fds[i].revents) {
      rc++;
    }
  }
  enable_interrupts();
  return rc;
}
