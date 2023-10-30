#ifndef SOCKET_H
#define SOCKET_H
#include <fs/fifo.h>
#include <fs/vfs.h>
#include <stddef.h>
#include <stdint.h>

#define AF_UNIX 0
#define AF_INET 1
#define AF_LOCAL AF_UNIX

#define SOCK_DGRAM 0

#define INADDR_ANY 0

#define MSG_WAITALL 1

typedef struct {
  vfs_fd_t *ptr_socket_fd;
  FIFO_FILE *fifo_file;

  int domain;
  int type;
  int protocol;
  void *child;

  // UNIX socket
  char *path;
  vfs_fd_t *incoming_fd;
} SOCKET;

typedef struct {
  char *path;
  SOCKET *s;
} OPEN_UNIX_SOCKET;

typedef struct {
  uint32_t address;
  uint16_t port;
  SOCKET *s;
} OPEN_INET_SOCKET;

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

OPEN_INET_SOCKET *find_open_udp_port(uint16_t port);
int uds_open(const char *path);
int socket(int domain, int type, int protocol);
int accept(int socket, struct sockaddr *address, socklen_t *address_len);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
#endif