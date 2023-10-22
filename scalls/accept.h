#include "../socket.h"

typedef struct SYS_ACCEPT_PARAMS {
  int socket;
  struct sockaddr *address;
  socklen_t *address_len;
} __attribute__((packed)) SYS_ACCEPT_PARAMS;

int syscall_accept(SYS_ACCEPT_PARAMS *args);
