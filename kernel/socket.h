#ifndef SOCKET_H
#define SOCKET_H
#include <fs/fifo.h>
#include <fs/vfs.h>
#include <lib/buffered_write.h>
#include <stddef.h>
#include <typedefs.h>

#define AF_UNIX 0
#define AF_INET 1
#define AF_LOCAL AF_UNIX

#define SOCK_DGRAM 0
#define SOCK_STREAM 1

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
  u32 address;
  u16 port;
  SOCKET *s;
} OPEN_INET_SOCKET;

struct INCOMING_TCP_CONNECTION {
  u8 ip[4];
  u16 n_port;
  u16 dst_port;
  FIFO_FILE *data_file;
  struct buffered buffer;
  u8 *has_data_ptr;
  u8 is_used;
  u32 ack_num;
  u32 seq_num;
  u8 connection_closed;
  u8 requesting_connection_close;
};

typedef u32 in_addr_t;
typedef u16 in_port_t;
typedef unsigned int sa_family_t;
typedef int socklen_t;

struct sockaddr {
  sa_family_t sa_family; /* Address family */
  char *sa_data;         /* Socket address */
};

struct sockaddr_in {
  sa_family_t sin_family;
  union {
    u32 s_addr;
    u8 a[4];
  } sin_addr;
  u16 sin_port;
};

struct sockaddr_un {
  sa_family_t sun_family; /* Address family */
  char *sun_path;         /* Socket pathname */
};

OPEN_INET_SOCKET *find_open_udp_port(u16 port);
OPEN_INET_SOCKET *find_open_tcp_port(u16 port);

int uds_open(const char *path);
int socket(int domain, int type, int protocol);
int accept(int socket, struct sockaddr *address, socklen_t *address_len);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
struct INCOMING_TCP_CONNECTION *
handle_incoming_tcp_connection(u8 ip[4], u16 n_port, u16 dst_port);
struct INCOMING_TCP_CONNECTION *get_incoming_tcp_connection(u8 ip[4],
                                                            u16 n_port);
#endif
