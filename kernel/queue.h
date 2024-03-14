#ifndef QUEUE_H
#define QUEUE_H
#include <lib/list.h>
#include <stddef.h>
#include <string.h>
#include <typedefs.h>

#define EVENT_TYPE_FD 0
#define EVENT_TYPE_TCP_SOCKET 1

struct event {
  u8 type; // File descriptor | Socket
  u32 internal_id;
};

struct event_queue {
  struct list events;
  int wait;
};

int queue_create(u32 *id);
int queue_add(u32 queue_id, struct event *ev, u32 size);
int queue_wait(u32 queue_id);
int queue_should_block(struct event_queue *q, int *is_empty);
#endif
