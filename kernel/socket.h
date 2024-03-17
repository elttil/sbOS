#ifndef SOCKET_H
#define SOCKET_H
#include <fs/fifo.h>
#include <fs/vfs.h>
#include <lib/buffered_write.h>
#include <lib/stack.h>
#include <stddef.h>
#include <typedefs.h>

#define AF_UNIX 0
#define AF_INET 1
#define AF_LOCAL AF_UNIX

#define SOCK_DGRAM 0
#define SOCK_STREAM 1

#define INADDR_ANY 0

#define MSG_WAITALL 1

void gen_ipv4(ipv4_t *ip, u8 i1, u8 i2, u8 i3, u8 i4);
u32 tcp_connect_ipv4(u32 ip, u16 port, int *error);

u32 tcp_listen_ipv4(u32 ip, u16 port, int *error);
u32 tcp_accept(u32 listen_socket, int *error);

int tcp_write(u32 socket, const u8 *buffer, u64 len, u64 *out);
int tcp_read(u32 socket, u8 *buffer, u64 buffer_size, u64 *out);

struct TcpListen {
  u32 ip;
  u16 port;
  struct stack incoming_connections;
};

struct TcpConnection {
  int dead;
  u16 incoming_port;
  u32 outgoing_ip;
  u16 outgoing_port;

  int unhandled_packet;

  FIFO_FILE *data_file;

  u32 seq;
  u32 ack;

  int handshake_state;
};

struct TcpConnection *tcp_get_connection(u32 socket, process_t *p);
struct TcpConnection *internal_tcp_incoming(u32 src_ip, u16 src_port,
                                            u32 dst_ip, u16 dst_port);

struct TcpConnection *tcp_find_connection(u16 port);

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
