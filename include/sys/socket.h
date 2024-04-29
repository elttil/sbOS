#ifndef SYS_SOCKET_H
#define SYS_SOCKET_H
#include <stddef.h>
#include <stdint.h>

#define AF_UNIX 0
#define AF_INET 1
#define AF_LOCAL AF_UNIX

#define SOCK_DGRAM 0
#define SOCK_STREAM 1
#define MSG_WAITALL 1

#define INADDR_ANY 0

#define IPPROTO_TCP 0
#define TCP_NODELAY 0

#ifndef KERNEL

typedef struct {
  int domain;
  int type;
  int protocol;

  // UNIX socket
  char *path;
  int incoming_fd;
} SOCKET;

typedef struct {
  char *path;
  SOCKET *s;
} OPEN_UNIX_SOCKET;

typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;
typedef unsigned int sa_family_t;
typedef int socklen_t;

struct sockaddr {
  sa_family_t sa_family; /* Address family */
  char *sa_data;         /* Socket address */
};

struct sockaddr_in {
  sa_family_t sin_family;
  union {
    uint32_t s_addr;
    uint8_t a[4];
  } sin_addr;
  uint16_t sin_port;
};

struct sockaddr_un {
  sa_family_t sun_family; /* Address family */
  char *sun_path;         /* Socket pathname */
};

int socket(int domain, int type, int protocol);
int accept(int socket, struct sockaddr *address, socklen_t *address_len);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
size_t recvfrom(int socket, void *buffer, size_t length, int flags,
                struct sockaddr *address, socklen_t *address_len);
size_t sendto(int socket, const void *message, size_t length, int flags,
              const struct sockaddr *dest_addr, socklen_t dest_len);
int listen(int socket, int backlog);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int setsockopt(int socket, int level, int option_name, const void *option_value,
               socklen_t option_len);
#endif // KERNEL
#endif // SYS_SOCKET_H
