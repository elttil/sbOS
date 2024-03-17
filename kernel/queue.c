#include <assert.h>
#include <kmalloc.h>
#include <queue.h>
#include <sched/scheduler.h>
#include <socket.h>
#include <stdio.h>

int queue_create(u32 *id, process_t *p) {
  struct event_queue *q = kcalloc(1, sizeof(struct event_queue));
  q->wait = 0;
  q->p = p;
  list_init(&q->events);

  struct list *list = &current_task->event_queue;
  list_add(list, q, id);
  return 1;
}

struct event_queue *get_event_queue(u32 id) {
  const struct list *list = &current_task->event_queue;
  struct event_queue *q;
  if (!list_get(list, id, (void **)&q)) {
    return NULL;
  }
  return q;
}

int queue_add(u32 queue_id, struct event *ev, u32 size) {
  struct event_queue *q = get_event_queue(queue_id);
  if (!q) {
    return 0;
  }
  for (u32 i = 0; i < size; i++) {
    // TODO: Should it be a copy or could it be kept in userland to
    // improve performance?
    struct event *new_ev = kmalloc(sizeof(struct event));
    memcpy(new_ev, ev, sizeof(struct event));
    list_add(&q->events, new_ev, NULL);
  }
  return 1;
}

int queue_should_block(struct event_queue *q, int *is_empty) {
  if (!q->wait) {
    return 0;
  }
  *is_empty = 1;
  for (int i = 0;; i++) {
    struct event *ev;
    if (!list_get(&q->events, i, (void **)&ev)) {
      break;
    }
    kprintf("wait %d\n", i);
    *is_empty = 0;
    if (EVENT_TYPE_FD == ev->type) {
      kprintf("found fd: %d\n", ev->internal_id);
      vfs_fd_t *fd = get_vfs_fd(ev->internal_id, q->p);
      kprintf("fd->inode->has_data: %x\n", fd->inode->has_data);
      if (!fd) {
        kprintf("queue: Invalid fd given\n");
        continue;
      }
      if (fd->inode->has_data) {
        kprintf("no block\n");
        return 0;
      }
    } else if (EVENT_TYPE_TCP_SOCKET == ev->type) {
      struct TcpConnection *con = tcp_get_connection(ev->internal_id, q->p);
      assert(con);
      assert(con->data_file);
      if (con->data_file->has_data) {
        kprintf("has data\n");
        return 0;
      } else {
        kprintf("blocking queue\n");
      }
    }
  }
  return 1;
}

int queue_wait(u32 queue_id) {
  struct event_queue *q = get_event_queue(queue_id);
  if (!q) {
    return 0;
  }
  q->wait = 1;
  switch_task();
  q->wait = 0;
  return 1;
}
