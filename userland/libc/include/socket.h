#include <stddef.h>
#include <stdint.h>

#define AF_UNIX 0
#define AF_LOCAL AF_UNIX

#define INADDR_ANY 0

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
typedef uint32_t socklen_t;

struct sockaddr {
  sa_family_t sa_family; /* Address family */
  char *sa_data;         /* Socket address */
};

struct sockaddr_un {
  sa_family_t sun_family; /* Address family */
  char *sun_path;         /* Socket pathname */
};

int socket(int domain, int type, int protocol);
int accept(int socket, struct sockaddr *address, socklen_t *address_len);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
