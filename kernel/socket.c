#include <assert.h>
#include <errno.h>
#include <fs/devfs.h>
#include <fs/tmpfs.h>
#include <interrupts.h>
#include <math.h>
#include <network/bytes.h>
#include <network/tcp.h>
#include <network/udp.h>
#include <poll.h>
#include <sched/scheduler.h>
#include <socket.h>
#include <sys/socket.h>

struct list open_udp_connections;
struct list open_tcp_connections;
struct list open_tcp_listen;

void global_socket_init(void) {
  list_init(&open_udp_connections);
  list_init(&open_tcp_connections);
  list_init(&open_tcp_listen);
}

OPEN_UNIX_SOCKET *un_sockets[100] = {0};

void gen_ipv4(ipv4_t *ip, u8 i1, u8 i2, u8 i3, u8 i4) {
  ip->a[0] = i1;
  ip->a[1] = i2;
  ip->a[2] = i3;
  ip->a[3] = i4;
}

struct TcpConnection *tcp_find_connection(ipv4_t src_ip, u16 src_port,
                                          u16 dst_port) {
  (void)src_ip;
  for (int i = 0;; i++) {
    struct TcpConnection *c;
    if (!list_get(&open_tcp_connections, i, (void **)&c)) {
      break;
    }
    if (c->incoming_port == dst_port && c->outgoing_port == src_port) {
      return c;
    }
  }
  return NULL;
}

struct UdpConnection *udp_find_connection(ipv4_t src_ip, u16 src_port,
                                          u16 dst_port) {
  (void)src_ip;
  for (int i = 0;; i++) {
    struct UdpConnection *c;
    if (!list_get(&open_udp_connections, i, (void **)&c)) {
      break;
    }
    if (c->dead) {
      continue;
    }
    if (c->incoming_port == dst_port && c->outgoing_port == src_port) {
      return c;
    }
  }
  return NULL;
}

struct TcpListen *tcp_find_listen(u16 port) {
  for (int i = 0;; i++) {
    struct TcpListen *c;
    if (!list_get(&open_tcp_listen, i, (void **)&c)) {
      break;
    }
    if (c->port == port) {
      return c;
    }
  }
  return NULL;
}

struct TcpConnection *internal_tcp_incoming(u32 src_ip, u16 src_port,
                                            u32 dst_ip, u16 dst_port) {
  struct TcpListen *listen = tcp_find_listen(dst_port);
  if (!listen) {
    return NULL;
  }

  struct TcpConnection *con = kcalloc(1, sizeof(struct TcpConnection));

  int connection_id;
  list_add(&open_tcp_connections, con, &connection_id);

  con->current_window_size = 536;
  con->outgoing_ip = src_ip;
  con->outgoing_port = src_port;
  con->incoming_ip = dst_ip;
  con->incoming_port = dst_port;
  con->no_delay = 0;

  ringbuffer_init(&con->incoming_buffer, 8192);
  ringbuffer_init(&con->outgoing_buffer, 8192);
  relist_init(&con->inflight);
  con->max_inflight = 1;
  stack_push(&listen->incoming_connections, con);
  return con;
}

int tcp_sync_buffer(vfs_fd_t *fd) {
  struct TcpConnection *con = fd->inode->internal_object;
  assert(con);
  if (con->dead) {
    return 0;
  }

  struct ringbuffer *rb = &con->outgoing_buffer;
  u32 send_buffer_len = ringbuffer_used(rb);
  if (0 == send_buffer_len) {
    return 0;
  }

  send_buffer_len = min(tcp_can_send(con), send_buffer_len);
  if(0 == send_buffer_len) {
    return 0;
  }

  char *send_buffer = kmalloc(send_buffer_len);
  assert(ringbuffer_read(rb, send_buffer, send_buffer_len) == send_buffer_len);
  send_tcp_packet(con, send_buffer, send_buffer_len);
  kfree(send_buffer);
  return 1;
}

void tcp_close(vfs_fd_t *fd) {
  struct TcpConnection *con = fd->inode->internal_object;

  tcp_sync_buffer(fd);

  tcp_close_connection(con);
}

int tcp_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  struct TcpConnection *con = fd->inode->internal_object;
  assert(con);
  if (con->dead) {
    return -EBADF; // TODO: Check if this is correct.
  }

  u32 rc = ringbuffer_read(&con->incoming_buffer, buffer, len);
  if (0 == rc && len > 0) {
    return -EWOULDBLOCK;
  }
  return rc;
}

int tcp_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  (void)offset;
  struct TcpConnection *con = fd->inode->internal_object;
  assert(con);
  if (con->dead) {
    return -EBADF; // TODO: Check if this is correct.
  }

  len = min(len, tcp_can_send(con));
  if (0 == len) {
    return -EWOULDBLOCK;
  }

  if (con->no_delay) {
    if (!send_tcp_packet(con, buffer, len)) {
      return -EWOULDBLOCK;
    }
    return len;
  }

  struct ringbuffer *rb = &con->outgoing_buffer;
  if (ringbuffer_unused(rb) < len) {
    if (!tcp_sync_buffer(fd)) {
      return -EWOULDBLOCK;
    }
    if (!send_tcp_packet(con, buffer, len)) {
      return -EWOULDBLOCK;
    }
    len = 0;
  } else {
    assert(ringbuffer_write(rb, buffer, len) == len);
  }
  return len;
}

int tcp_has_data(vfs_inode_t *inode) {
  struct TcpConnection *con = inode->internal_object;
  return !(ringbuffer_isempty(&con->incoming_buffer));
}

int tcp_can_write(vfs_inode_t *inode) {
  struct TcpConnection *con = inode->internal_object;
  if (con->no_delay) {
    return (0 != tcp_can_send(con));
  }
  return (ringbuffer_unused(&con->outgoing_buffer) > 0) ||
         (0 != tcp_can_send(con));
}

int udp_recvfrom(vfs_fd_t *fd, void *buffer, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
  struct UdpConnection *con = fd->inode->internal_object;
  assert(con);
  if (con->dead) {
    return -EBADF; // TODO: Check if this is correct.
  }

  u32 packet_length;
  u32 rc = ringbuffer_read(&con->incoming_buffer, (void *)&packet_length,
                           sizeof(u32));
  if (0 == rc) {
    return -EWOULDBLOCK;
  }
  assert(sizeof(u32) == rc);
  assert(sizeof(struct sockaddr_in) <= packet_length);

  struct sockaddr_in from;
  ringbuffer_read(&con->incoming_buffer, (void *)&from, sizeof(from));

  if (addrlen) {
    if (src_addr) {
      memcpy(src_addr, &from, min(sizeof(from), *addrlen));
    }
    *addrlen = sizeof(from);
  }

  u32 rest = packet_length - sizeof(from);
  u32 to_read = min(rest, len);
  rest = rest - to_read;

  rc = ringbuffer_read(&con->incoming_buffer, buffer, to_read);
  // Discard the rest of the packet
  // FIXME Maybe the rest of the packet can be kept? But unsure how that
  // would be done while preserving the header and not messing up other
  // packets.
  (void)ringbuffer_read(&con->incoming_buffer, NULL, rest);
  return rc;
}

int udp_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  return udp_recvfrom(fd, buffer, len, 0, NULL, NULL);
}

int udp_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  (void)offset;
  struct UdpConnection *con = fd->inode->internal_object;
  assert(con);
  if (con->dead) {
    return -EBADF; // TODO: Check if this is correct.
  }

  struct sockaddr_in dst;
  dst.sin_addr.s_addr = con->outgoing_ip;
  dst.sin_port = htons(con->outgoing_port);
  struct sockaddr_in src;
  src.sin_addr.s_addr = con->incoming_ip;
  src.sin_port = htons(con->incoming_port);
  send_udp_packet(&src, &dst, buffer, (u16)len);
  return len;
}

int udp_has_data(vfs_inode_t *inode) {
  struct UdpConnection *con = inode->internal_object;
  return !(ringbuffer_isempty(&con->incoming_buffer));
}

void udp_close(vfs_fd_t *fd) {
  struct UdpConnection *con = fd->inode->internal_object;
  con->dead = 1;
  ringbuffer_free(&con->incoming_buffer);
  return;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  vfs_fd_t *fd = get_vfs_fd(sockfd, NULL);
  if (!fd) {
    return -EBADF;
  }
  assert(fd->inode);
  assert(fd->inode->connect);
  return fd->inode->connect(fd, addr, addrlen);
}

int udp_connect(vfs_fd_t *fd, const struct sockaddr *addr, socklen_t addrlen) {
  assert(AF_INET == addr->sa_family);
  const struct sockaddr_in *in_addr = (const struct sockaddr_in *)addr;
  struct UdpConnection *con = kcalloc(1, sizeof(struct UdpConnection));
  if (!con) {
    return -ENOMEM;
  }

  con->incoming_port = 1337; // TODO
  con->outgoing_ip = in_addr->sin_addr.s_addr;
  con->outgoing_port = ntohs(in_addr->sin_port);
  assert(ringbuffer_init(&con->incoming_buffer, 8192));

  assert(list_add(&open_udp_connections, con, NULL));

  fd->inode->_has_data = udp_has_data;
  fd->inode->write = udp_write;
  fd->inode->read = udp_read;
  fd->inode->close = udp_close;
  fd->inode->internal_object = con;
  return 0;
}

int tcp_connect(vfs_fd_t *fd, const struct sockaddr *addr, socklen_t addrlen) {
  assert(AF_INET == addr->sa_family);
  const struct sockaddr_in *in_addr = (const struct sockaddr_in *)addr;
  struct TcpConnection *con = kcalloc(1, sizeof(struct TcpConnection));
  if (!con) {
    return -ENOMEM;
  }

  con->incoming_port = 1337; // TODO
  con->outgoing_ip = in_addr->sin_addr.s_addr;
  con->outgoing_port = ntohs(in_addr->sin_port);
  con->current_window_size = 536;

  ringbuffer_init(&con->incoming_buffer, 8192);
  ringbuffer_init(&con->outgoing_buffer, 8192);
  list_add(&open_tcp_connections, con, NULL);
  relist_init(&con->inflight);
  con->max_inflight = 1;

  tcp_send_syn(con);

  for (;;) {
    tcp_wait_reply(con);
    if (con->dead) { // Port is probably closed
      return -ECONNREFUSED;
    }
    if (0 != con->handshake_state) {
      break;
    }
  }

  fd->inode->_has_data = tcp_has_data;
  fd->inode->_can_write = tcp_can_write;
  fd->inode->write = tcp_write;
  fd->inode->read = tcp_read;
  fd->inode->close = tcp_close;
  fd->inode->internal_object = con;
  return 0;
}

int setsockopt(int socket, int level, int option_name, const void *option_value,
               socklen_t option_len) {
  vfs_fd_t *fd = get_vfs_fd(socket, NULL);
  if (!fd) {
    return -EBADF;
  }
  // TODO: Check that it is actually a TCP socket
  if (IPPROTO_TCP == level) {
    struct TcpConnection *tcp_connection = fd->inode->internal_object;
    switch (option_name) {
    case TCP_NODELAY: {
      if (sizeof(int) != option_len) {
        return -EINVAL;
      }
      if (!mmu_is_valid_userpointer(option_value, option_len)) {
        return -EPERM; // TODO: Check if this is correct
      }
      int value = *(int *)option_value;
      tcp_connection->no_delay = value;
    } break;
    default:
      return -ENOPROTOOPT;
      break;
    }
    return 0;
  }
  return -ENOPROTOOPT;
}

int uds_open(const char *path) {
  // FIXME: This is super ugly

  // Find the socket that path belongs to
  SOCKET *s = NULL;
  for (int i = 0; i < 100; i++) {
    if (!un_sockets[i]) {
      continue;
    }
    const char *p = path;
    const char *e = p;
    for (; *e; e++)
      ;
    for (; e != p && *e != '/'; e--)
      ;
    if (0 == strcmp(e, un_sockets[i]->path)) {
      s = un_sockets[i]->s;
      break;
    }
  }
  if (!s) {
    return -1;
  }

  // Create a pipe
  int fd[2];
  dual_pipe(fd);

  char c = 'i';
  fifo_object_write((u8 *)&c, 1, 0, s->fifo_file);

  s->incoming_fd = get_vfs_fd(fd[1], NULL);
  s->incoming_fd->reference_count++;
  vfs_close(fd[1]);
  return fd[0];
}

int tcp_listen_has_data(vfs_inode_t *inode) {
  const SOCKET *s = (SOCKET *)inode->internal_object;
  const struct TcpListen *tcp_listen = s->object;
  return !stack_isempty(&tcp_listen->incoming_connections);
}

int accept(int socket, struct sockaddr *address, socklen_t *address_len) {
  (void)address;
  (void)address_len;
  vfs_fd_t *fd_ptr = get_vfs_fd(socket, NULL);
  assert(fd_ptr);
  vfs_inode_t *inode = fd_ptr->inode;
  SOCKET *s = (SOCKET *)inode->internal_object;
  if (AF_UNIX == s->domain) {
    for (; NULL == s->incoming_fd;) {
      // Wait until we have gotten a connection
      struct pollfd fds[1];
      fds[0].fd = socket;
      fds[0].events = POLLIN;
      fds[0].revents = 0;
      poll(fds, 1, 0);
    }

    int index;
    assert(relist_add(&current_task->file_descriptors, s->incoming_fd, &index));
    assert(1 <= s->incoming_fd->reference_count);
    s->incoming_fd = NULL;
    return index;
  }
  if (AF_INET == s->domain) {
    struct TcpListen *tcp_listen = s->object;
    assert(tcp_listen);
    if (stack_isempty(&tcp_listen->incoming_connections)) {
      if (fd_ptr->flags & O_NONBLOCK) {
        return -EWOULDBLOCK;
      }
      struct pollfd fds[1];
      fds[0].fd = socket;
      fds[0].events = POLLIN;
      fds[0].revents = 0;
      poll(fds, 1, 0);
    }
    struct TcpConnection *connection =
        stack_pop(&tcp_listen->incoming_connections);
    assert(connection);
    vfs_inode_t *inode = vfs_create_inode(
        0 /*inode_num*/, FS_TYPE_UNIX_SOCKET, tcp_has_data, tcp_can_write, 1,
        connection, 0 /*file_size*/, NULL /*open*/, NULL /*create_file*/,
        tcp_read, tcp_write, tcp_close /*close*/, NULL /*create_directory*/,
        NULL /*get_vm_object*/, NULL /*truncate*/, NULL /*stat*/,
        NULL /*send_signal*/, NULL /*connect*/);
    assert(inode);
    return vfs_create_fd(O_RDWR, 0, 0 /*is_tty*/, inode, NULL);
  }
  ASSERT_NOT_REACHED;
  return 0;
}

int bind_has_data(vfs_inode_t *inode) {
  SOCKET *socket = inode->internal_object;
  return socket->incoming_fd;
}

int bind_can_write(vfs_inode_t *inode) {
  (void)inode;
  return 1;
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  (void)addrlen;
  vfs_fd_t *fd = get_vfs_fd(sockfd, NULL);
  if (!fd) {
    return -EBADF;
  }
  vfs_inode_t *inode = fd->inode;
  if (!inode) {
    return -EBADF;
  }
  if (FS_TYPE_UNIX_SOCKET != inode->type) {
    return -ENOTSOCK;
  }
  SOCKET *s = (SOCKET *)inode->internal_object;
  if (AF_UNIX == s->domain) {
    struct sockaddr_un *un = (struct sockaddr_un *)addr;
    size_t path_len = strlen(un->sun_path);
    s->path = kmalloc(path_len + 1);
    memcpy(s->path, un->sun_path, path_len);
    s->path[path_len] = '\0';

    OPEN_UNIX_SOCKET *us;
    int i = 0;
    for (; i < 100; i++) {
      if (!un_sockets[i]) {
        break;
      }
    }

    us = un_sockets[i] = kmalloc(sizeof(OPEN_UNIX_SOCKET));

    us->path = s->path;
    us->s = s;
    s->child = us;
    devfs_add_file(us->path, NULL, NULL, NULL, NULL, bind_can_write,
                   FS_TYPE_UNIX_SOCKET);
    return 0;
  } else if (AF_INET == s->domain) {
    struct sockaddr_in *in = (struct sockaddr_in *)addr;

    struct TcpListen *tcp_listen = kmalloc(sizeof(struct TcpListen));
    if (!tcp_listen) {
      return -ENOMEM;
    }
    tcp_listen->ip = in->sin_addr.s_addr;
    tcp_listen->port = ntohs(in->sin_port);
    stack_init(&tcp_listen->incoming_connections);

    s->object = tcp_listen;

    inode->_has_data = tcp_listen_has_data;

    list_add(&open_tcp_listen, tcp_listen, NULL);
    return 0;
  }
  ASSERT_NOT_REACHED;
}

int socket_has_data(vfs_inode_t *inode) {
  SOCKET *s = (SOCKET *)inode->internal_object;
  FIFO_FILE *file = s->fifo_file;
  return file->has_data;
}

int socket_can_write(vfs_inode_t *inode) {
  (void)inode;
  return 1;
}

int socket_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  SOCKET *s = (SOCKET *)fd->inode->internal_object;
  FIFO_FILE *file = s->fifo_file;
  return fifo_object_write(buffer, 0, len, file);
}

int socket_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  SOCKET *s = (SOCKET *)fd->inode->internal_object;
  FIFO_FILE *file = s->fifo_file;
  return fifo_object_read(buffer, 0, len, file);
}

void socket_close(vfs_fd_t *fd) {
  fd->inode->is_open = 0;
}

int socket(int domain, int type, int protocol) {
  int rc = 0;
  vfs_inode_t *inode = NULL;
  SOCKET *new_socket = NULL;
  if (!(AF_UNIX == domain || AF_INET == domain)) {
    rc = -EINVAL;
    goto socket_error;
  }

  new_socket = kmalloc(sizeof(SOCKET));
  if (!new_socket) {
    rc = -ENOMEM;
    goto socket_error;
  }
  new_socket->domain = domain;
  new_socket->type = type;
  new_socket->protocol = protocol;
  new_socket->path = NULL;
  new_socket->incoming_fd = NULL;
  if (AF_UNIX == domain) {
    vfs_inode_t *inode = vfs_create_inode(
        0 /*inode_num*/, FS_TYPE_UNIX_SOCKET, bind_has_data, socket_can_write,
        1 /*is_open*/, new_socket /*internal_object*/, 0 /*file_size*/,
        NULL /*open*/, NULL /*create_file*/, socket_read, socket_write,
        socket_close, NULL /*create_directory*/, NULL /*get_vm_object*/,
        NULL /*truncate*/, NULL /*stat*/, NULL /*send_signal*/,
        NULL /*connect*/);
    if (!inode) {
      rc = -ENOMEM;
      goto socket_error;
    }

    vfs_fd_t *fd;
    int n = vfs_create_fd(O_RDWR | O_NONBLOCK, 0, 0 /*is_tty*/, inode, &fd);
    if (n < 0) {
      rc = n;
      goto socket_error;
    }

    new_socket->fifo_file = create_fifo_object();

    new_socket->ptr_socket_fd = fd;
    return n;
  }
  if (AF_INET == domain) {
    int is_udp = (SOCK_DGRAM == type);

    void *connect_handler = NULL;
    if (is_udp) {
      connect_handler = udp_connect;
    } else {
      connect_handler = tcp_connect;
    }

    vfs_inode_t *inode = vfs_create_inode(
        0 /*inode_num*/, FS_TYPE_UNIX_SOCKET, NULL, always_can_write, 1,
        new_socket, 0 /*file_size*/, NULL /*open*/, NULL /*create_file*/, NULL,
        NULL, NULL /*close*/, NULL /*create_directory*/, NULL /*get_vm_object*/,
        NULL /*truncate*/, NULL /*stat*/, NULL /*send_signal*/,
        connect_handler);
    if (!inode) {
      rc = -ENOMEM;
      goto socket_error;
    }
    int n = vfs_create_fd(O_RDWR, 0, 0 /*is_tty*/, inode, NULL);
    if (n < 0) {
      rc = n;
      goto socket_error;
    }
    return n;
  }
  ASSERT_NOT_REACHED;
socket_error:
  kfree(inode);
  kfree(new_socket);
  return rc;
}
