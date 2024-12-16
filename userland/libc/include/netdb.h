#include <arpa/inet.h>
#include <sys/socket.h>

#define EAI_AGAIN -1
#define EAI_BADFLAGS -2
#define EAI_FAIL -3
#define EAI_FAMILY -4
#define EAI_MEMORY -5
#define EAI_NONAME -6
#define EAI_SERVICE -7
#define EAI_SOCKTYPE -8
#define EAI_SYSTEM -9
#define EAI_OVERFLOW -10

int getaddrinfo(const char *restrict node, const char *restrict service,
                const struct addrinfo *restrict hints,
                struct addrinfo **restrict res);
void freeaddrinfo(struct addrinfo *res);
const char *gai_strerror(int errcode);
