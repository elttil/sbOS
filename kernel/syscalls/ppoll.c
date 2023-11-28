#include <fs/vfs.h>
#include <poll.h>
#include <sched/scheduler.h>
#include <syscalls.h>

int syscall_poll(SYS_POLL_PARAMS *args) {
  struct pollfd *fds = args->fds;
  size_t nfds = args->nfds;
  int timeout = args->timeout;
  return poll(fds, nfds, timeout);
}
