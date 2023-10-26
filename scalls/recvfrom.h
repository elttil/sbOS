#include <socket.h>

size_t syscall_recvfrom(int socket, void *buffer, size_t length, int flags,
                         struct sockaddr *address, socklen_t *address_len);
