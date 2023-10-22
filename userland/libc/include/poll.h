#ifndef POLL_H
#define POLL_H
#include <stddef.h>

#define POLLIN (1 << 0)
#define POLLPRI (1 << 1)
#define POLLOUT (1 << 2)
#define POLLHUP (1 << 3)

struct pollfd {
  int fd;
  short events;
  short revents;
};

int poll(struct pollfd *fds, size_t nfds, int timeout);
#endif
