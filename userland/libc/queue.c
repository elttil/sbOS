#include <queue.h>
#include <syscall.h>

int queue_create(void) {
  RC_ERRNO(syscall(SYS_QUEUE_CREATE, 0, 0, 0, 0, 0));
}

int queue_mod_entries(int fd, int flag, struct queue_entry *entries,
                      int num_entries) {
  RC_ERRNO(syscall(SYS_QUEUE_MOD_ENTRIES, fd, flag, entries, num_entries, 0));
}

int queue_wait(int fd, struct queue_entry *events, int num_events) {
  RC_ERRNO(syscall(SYS_QUEUE_WAIT, fd, events, num_events, 0, 0));
}
