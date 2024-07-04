struct sockaddr;
typedef int socklen_t;
#ifndef SOCKET_H
#define SOCKET_H
#include <fs/fifo.h>
#include <fs/vfs.h>
#include <lib/buffered_write.h>
#include <lib/relist.h>
#include <lib/ringbuffer.h>
#include <lib/stack.h>
#include <stddef.h>
#include <typedefs.h>

#define AF_UNIX 0
#define AF_INET 1
#define AF_LOCAL AF_UNIX

#define INADDR_ANY 0

#define MSG_WAITALL 1

void gen_ipv4(ipv4_t *ip, u8 i1, u8 i2, u8 i3, u8 i4);

struct TcpListen {
  u32 ip;
  u16 port;
  struct stack incoming_connections;
};

struct UdpConnection {
  u16 incoming_port;
  u32 incoming_ip;
  u32 outgoing_ip;
  u16 outgoing_port;

  int dead;

  struct ringbuffer incoming_buffer;
};

struct TcpConnection {

  int state;

  u16 incoming_port;
  u32 incoming_ip;
  u32 outgoing_ip;
  u16 outgoing_port;

  struct ringbuffer incoming_buffer;
  struct ringbuffer outgoing_buffer;

  int no_delay;

  u32 current_window_size;
  u32 window_size;
  u32 sent_ack;
  u32 recieved_ack;

  u32 max_seg;

  u32 rcv_wnd;
  u32 rcv_nxt;
  u32 rcv_adv;

  int should_send_ack;

  u32 snd_una;
  u32 snd_nxt;
  u32 snd_max;
  u32 snd_wnd;
};

struct TcpConnection *tcp_find_connection(ipv4_t src_ip, u16 src_port,
                                          ipv4_t dst_ip, u16 dst_port);
struct UdpConnection *udp_find_connection(ipv4_t src_ip, u16 src_port,
                                          u16 dst_port);

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

  void *object;
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
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
void global_socket_init(void);
u16 tcp_get_free_port(void);
int setsockopt(int socket, int level, int option_name, const void *option_value,
               socklen_t option_len);
void tcp_remove_connection(struct TcpConnection *con);
void tcp_flush_acks(void);
#endif
