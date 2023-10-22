#ifndef SOCKET_H
#define SOCKET_H
#include "fs/vfs.h"
#include <stddef.h>
#include <stdint.h>

/*
// Create a directory with the proper permissions
mkdir(path, 0700);
// Append the name of the socket
strcat(path, "/socket_name");

// Create the socket normally
sockaddr_un address;
address.sun_family = AF_UNIX;
strcpy(address.sun_path, path);
int fd = socket(AF_UNIX, SOCK_STREAM, 0);
bind(fd, (sockaddr*)(&address), sizeof(address));
listen(fd, 100);
*/

#define AF_UNIX 0
#define AF_LOCAL AF_UNIX

#define INADDR_ANY 0

typedef struct {
  int fifo_fd;
  vfs_fd_t *ptr_fifo_fd;

  vfs_fd_t *ptr_socket_fd;

  int domain;
  int type;
  int protocol;

  // UNIX socket
  char *path;
  vfs_fd_t *incoming_fd;
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

int uds_open(const char *path);
int socket(int domain, int type, int protocol);
int accept(int socket, struct sockaddr *address, socklen_t *address_len);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
#endif
