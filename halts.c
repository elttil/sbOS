#include <assert.h>
#include <halts.h>
#include <sched/scheduler.h>

int create_disconnect_inode_halt(vfs_inode_t *inode) {
  volatile process_t *p = get_current_task();
  int i;
  for (i = 0; i < 100; i++)
    if (!p->disconnect_halt_inode[i])
      break;

  if (p->disconnect_halt_inode[i])
    return -1;

  p->disconnect_halt_inode[i] = inode;

  return i;
}

int create_disconnect_fdhalt(vfs_fd_t *fd) {
  assert(fd);
  return create_disconnect_inode_halt(fd->inode);
}

int create_read_inode_halt(vfs_inode_t *inode) {
  volatile process_t *p = get_current_task();
  int i;
  for (i = 0; i < 100; i++)
    if (!p->read_halt_inode[i])
      break;

  if (p->read_halt_inode[i])
    return -1;

  p->read_halt_inode[i] = inode;

  return i;
}

int create_read_fdhalt(vfs_fd_t *fd) {
  assert(fd);
  return create_read_inode_halt(fd->inode);
}

int create_write_inode_halt(vfs_inode_t *inode) {
  volatile process_t *p = get_current_task();
  int i;
  for (i = 0; i < 100; i++)
    if (!p->write_halt_inode[i])
      break;

  if (p->write_halt_inode[i])
    return -1;

  p->write_halt_inode[i] = inode;

  return i;
}

int create_write_fdhalt(vfs_fd_t *fd) {
  return create_write_inode_halt(fd->inode);
}

void unset_read_fdhalt(int i) { get_current_task()->read_halt_inode[i] = NULL; }

void unset_write_fdhalt(int i) {
  get_current_task()->write_halt_inode[i] = NULL;
}

void unset_disconnect_fdhalt(int i) {
  get_current_task()->disconnect_halt_inode[i] = NULL;
}

int isset_fdhalt(vfs_inode_t *read_halts[], vfs_inode_t *write_halts[],
                 vfs_inode_t *disconnect_halts[]) {
  int blocked = 0;
  for (int i = 0; i < 100; i++) {
    if (!read_halts[i])
      continue;
    if (read_halts[i]->has_data) {
      return 0;
    }
    blocked = 1;
  }
  for (int i = 0; i < 100; i++) {
    if (!write_halts[i])
      continue;
    if (write_halts[i]->can_write) {
      return 0;
    }
    blocked = 1;
  }
  for (int i = 0; i < 100; i++) {
    if (!disconnect_halts[i])
      continue;
    if (!disconnect_halts[i]->is_open) {
      return 0;
    }
    blocked = 1;
  }
  return blocked;
}
