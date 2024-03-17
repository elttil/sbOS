#include <queue.h>
#include <syscalls.h>

int syscall_queue_create(u32 *id) {
  return queue_create(id, current_task);
}

int syscall_queue_add(u32 queue_id, struct event *ev, u32 size) {
  return queue_add(queue_id, ev, size);
}

int syscall_queue_wait(u32 queue_id) {
  return queue_wait(queue_id);
}
