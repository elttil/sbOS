#include "accept.h"

int syscall_accept(SYS_ACCEPT_PARAMS *args) {
  return accept(args->socket, args->address, args->address_len);
}
