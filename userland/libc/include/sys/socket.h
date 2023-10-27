#include <socket.h>
#include <stddef.h>

#define MSG_WAITALL 1

size_t recvfrom(int socket, void *buffer, size_t length, int flags,
                struct sockaddr *address, socklen_t *address_len);
size_t sendto(int socket, const void *message, size_t length, int flags,
               const struct sockaddr *dest_addr, socklen_t dest_len);
