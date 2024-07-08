#include <assert.h>
#include <fs/vfs.h>
#include <queue.h>

#define OBJECT_QUEUE 0x1337

void queue_close(vfs_fd_t *inode) {
  // TODO
  (void)inode;
}

int queue_get_entries(struct queue_list *list, struct queue_entry *events,
                      int num_events) {
  int rc = 0;
  for (int i = 0; rc < num_events; i++) {
    struct queue_entry *entry;
    int end;
    if (!relist_get(&list->entries, i, (void **)&entry, &end)) {
      if (end) {
        break;
      }
      continue;
    }
    if (0 == entry->listen) {
      continue;
    }
    vfs_fd_t *ptr = get_vfs_fd(entry->fd, list->process);
    if (!ptr) {
      continue;
    }
    int should_add = 0;
    if (QUEUE_WAIT_READ & entry->listen) {
      if (ptr->inode->_has_data) {
        if (ptr->inode->_has_data(ptr->inode)) {
          should_add = 1;
        }
      }
    }
    if (QUEUE_WAIT_WRITE & entry->listen) {
      if (ptr->inode->_can_write) {
        if (ptr->inode->_can_write(ptr->inode)) {
          should_add = 1;
        }
      }
    }
    if (QUEUE_WAIT_CLOSE & entry->listen) {
      if (ptr->inode->_is_open) {
        if (!ptr->inode->_is_open(ptr->inode)) {
          should_add = 1;
        }
      }
    }
    if (should_add) {
      if (events) {
        memcpy(events + rc, entry, sizeof(struct queue_entry));
      }
      rc++;
    }
  }
  return rc;
}

int queue_has_data(vfs_inode_t *inode) {
  if (1 == queue_get_entries(inode->internal_object, NULL, 1)) {
    return 1;
  }
  return 0;
}

int queue_create(void) {
  struct queue_list *list = kmalloc(sizeof(struct queue_list));
  assert(list);
  relist_init(&list->entries);
  list->process = current_task;

  vfs_inode_t *inode =
      vfs_create_inode(0, 0, queue_has_data, NULL, 1 /*is_open*/, OBJECT_QUEUE,
                       list /*internal_object*/, 0, NULL, NULL, NULL, NULL,
                       queue_close, NULL, NULL /*get_vm_object*/, NULL, NULL,
                       NULL /*send_signal*/, NULL /*connect*/);
  assert(inode);
  return vfs_create_fd(0, 0, 0, inode, NULL);
}

int queue_mod_entries(int fd, int flag, struct queue_entry *entries,
                      int num_entries) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd, NULL);
  assert(fd_ptr);
  assert(OBJECT_QUEUE == fd_ptr->inode->internal_object_type);
  struct queue_list *list = fd_ptr->inode->internal_object;
  if (QUEUE_MOD_ADD == flag) {
    int i = 0;
    for (; i < num_entries; i++) {
      struct queue_entry *copy = kmalloc(sizeof(struct queue_entry));
      if (!copy) {
        break;
      }
      memcpy(copy, entries + i, sizeof(struct queue_entry));
      if (!relist_add(&list->entries, copy, NULL)) {
        kfree(copy);
        break;
      }
    }
    return i;
  } else if (QUEUE_MOD_CHANGE == flag) {
    int changes = 0;
    for (int i = 0; changes < num_entries; i++) {
      struct queue_entry *entry;
      int end;
      if (!relist_get(&list->entries, i, (void **)&entry, &end)) {
        if (end) {
          break;
        }
        continue;
      }
      for (int j = 0; j < num_entries; j++) {
        if (entry->fd == entries[j].fd) {
          entry->listen = entries[j].listen;
          entry->data_type = entries[j].data_type;
          entry->data = entries[j].data;
          changes++;
          break;
        }
      }
    }
    return changes;
  } else if (QUEUE_MOD_DELETE == flag) {
    int changes = 0;
    for (int i = 0; changes < num_entries; i++) {
      struct queue_entry *entry;
      int end;
      if (!relist_get(&list->entries, i, (void **)&entry, &end)) {
        if (end) {
          break;
        }
        continue;
      }
      for (int j = 0; j < num_entries; j++) {
        if (entry->fd == entries[j].fd) {
          assert(relist_remove(&list->entries, i));
          kfree(entry);
          break;
        }
      }
    }
    return changes;
  } else {
    assert(0);
  }
  return 0;
}

int queue_wait(int fd, struct queue_entry *events, int num_events) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd, NULL);
  assert(fd_ptr);
  assert(OBJECT_QUEUE == fd_ptr->inode->internal_object_type);
  struct queue_list *list = fd_ptr->inode->internal_object;
  int rc = queue_get_entries(list, events, num_events);
  if (0 == rc) {
    list_add(&current_task->read_list, fd_ptr->inode, NULL);
    switch_task();
    list_reset(&current_task->read_list);
    rc = queue_get_entries(list, events, num_events);
  }
  return rc;
}
