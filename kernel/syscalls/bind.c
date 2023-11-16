#include <syscalls.h>

int syscall_bind(SYS_BIND_PARAMS *args) {
  return bind(args->sockfd, args->addr, args->addrlen);
}
