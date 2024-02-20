#include <assert.h>
#include <halts.h>
#include <sched/scheduler.h>

int isset_fdhalt(process_t *p) {
  int blocked = 0;
  struct list *read_list = &p->read_list;
  struct list *write_list = &p->write_list;
  struct list *disconnect_list = &p->disconnect_list;

  for (int i = 0;; i++) {
    vfs_inode_t *inode;
    if (!list_get(read_list, i, (void **)&inode)) {
      break;
    }
    if (inode->has_data) {
      return 0;
    }
    blocked = 1;
  }
  for (int i = 0;; i++) {
    vfs_inode_t *inode;
    if (!list_get(write_list, i, (void **)&inode)) {
      break;
    }
    if (inode->can_write) {
      return 0;
    }
    blocked = 1;
  }
  for (int i = 0;; i++) {
    vfs_inode_t *inode;
    if (!list_get(disconnect_list, i, (void **)&inode)) {
      break;
    }
    if (inode->is_open) {
      return 0;
    }
    blocked = 1;
  }
  return blocked;
}
