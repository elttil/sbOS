#include <socket.h>

struct two_args {
  uint32_t a;
  uint32_t b;
};

size_t syscall_recvfrom(
    int socket, void *buffer, size_t length, int flags,
    struct two_args
        *extra_args /*struct sockaddr *address, socklen_t *address_len*/);
