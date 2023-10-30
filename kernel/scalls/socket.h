#include "../socket.h"

typedef struct SYS_SOCKET_PARAMS {
  int domain;
  int type;
  int protocol;
} __attribute__((packed)) SYS_SOCKET_PARAMS;

int syscall_socket(SYS_SOCKET_PARAMS *args);
