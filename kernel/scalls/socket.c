#include "socket.h"

int syscall_socket(SYS_SOCKET_PARAMS *args) {
  return socket(args->domain, args->type, args->protocol);
}
