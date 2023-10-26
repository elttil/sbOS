#include <socket.h>
#include <stddef.h>

#define MSG_WAITALL 1

size_t recvfrom(int socket, void *buffer, size_t length, int flags,
                struct sockaddr *address, socklen_t *address_len);
