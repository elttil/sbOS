#include "../socket.h"

typedef struct SYS_BIND_PARAMS {
  int sockfd;
  const struct sockaddr *addr;
  socklen_t addrlen;
} __attribute__((packed)) SYS_BIND_PARAMS;

int syscall_bind(SYS_BIND_PARAMS *args);
