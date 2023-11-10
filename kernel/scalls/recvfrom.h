#include <socket.h>

struct two_args {
  u32 a;
  u32 b;
};

size_t syscall_recvfrom(
    int socket, void *buffer, size_t length, int flags,
    struct two_args
        *extra_args /*struct sockaddr *address, socklen_t *address_len*/);
