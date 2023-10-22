#include <stddef.h>

typedef struct SYS_POLL_PARAMS {
  struct pollfd *fds;
  size_t nfds;
  int timeout;
} __attribute__((packed)) SYS_POLL_PARAMS;

int syscall_poll(SYS_POLL_PARAMS *args);
