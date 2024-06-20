#include <arpa/inet.h>
#include <sys/socket.h>

int getaddrinfo(const char *restrict node, const char *restrict service,
                const struct addrinfo *restrict hints,
                struct addrinfo **restrict res);
void freeaddrinfo(struct addrinfo *res);
