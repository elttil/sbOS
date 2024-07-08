#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fs/devfs.h>
#include <fs/tmpfs.h>
#include <interrupts.h>
#include <lib/ringbuffer.h>
#include <lib/stack.h>
#include <math.h>
#include <network/bytes.h>
#include <network/tcp.h>
#include <network/udp.h>
#include <poll.h>
#include <sched/scheduler.h>
#include <socket.h>
#include <stdatomic.h>
#include <sys/socket.h>

#define OBJECT_UNIX 0
#define OBJECT_TCP 1

struct list open_udp_connections;
struct relist open_tcp_connections;

struct TcpConnection *tcp_get_incoming_connection(struct TcpConnection *con,
                                                  int remove);

void global_socket_init(void) {
  list_init(&open_udp_connections);
  relist_init(&open_tcp_connections);
}

OPEN_UNIX_SOCKET *un_sockets[100] = {0};

void gen_ipv4(ipv4_t *ip, u8 i1, u8 i2, u8 i3, u8 i4) {
  ip->d = (i1 << (8 * 0)) | (i2 << (8 * 1)) | (i3 << (8 * 2)) | (i4 << (8 * 3));
}

void tcp_remove_connection(struct TcpConnection *con) {
  for (int i = 0;; i++) {
    struct TcpConnection *c;
    int end;
    if (!relist_get(&open_tcp_connections, i, (void **)&c, &end)) {
      if (end) {
        break;
      }
      continue;
    }
    if (c == con) {
      relist_remove(&open_tcp_connections, i);
      ringbuffer_free(&c->incoming_buffer);
      ringbuffer_free(&c->outgoing_buffer);
      kfree(con);
      break;
    }
  }
}

int num_open_connections(void) {
  int f = 0;
  for (int i = 0;; i++) {
    struct TcpConnection *c;
    int end;
    if (!relist_get(&open_tcp_connections, i, (void **)&c, &end)) {
      if (end) {
        break;
      }
      continue;
    }
    f++;
  }
  return f;
}

struct TcpConnection *tcp_connect_to_listen(ipv4_t src_ip, u16 src_port,
                                            ipv4_t dst_ip, u16 dst_port) {
  for (int i = 0;; i++) {
    struct TcpConnection *c;
    int end;
    if (!relist_get(&open_tcp_connections, i, (void **)&c, &end)) {
      if (end) {
        break;
      }
      continue;
    }
    if (TCP_STATE_CLOSED == c->state) {
      continue;
    }
    if (TCP_STATE_LISTEN != c->state) {
      continue;
    }
    if (c->incoming_port == dst_port) {
      struct TcpConnection *new_connection =
          kmalloc(sizeof(struct TcpConnection));
      new_connection->incoming_port = c->incoming_port;
      new_connection->incoming_ip = c->incoming_ip;

      new_connection->no_delay = c->no_delay;

      new_connection->outgoing_port = src_port;
      new_connection->outgoing_ip = src_ip.d;
      new_connection->state = TCP_STATE_LISTEN;
      new_connection->rcv_wnd = 65535;

      ringbuffer_init(&new_connection->outgoing_buffer, 8192);
      ringbuffer_init(&new_connection->incoming_buffer,
                      new_connection->rcv_wnd * 64);
      new_connection->max_seg = 1;

      new_connection->rcv_nxt = 0;
      new_connection->rcv_adv = 0;

      new_connection->snd_una = 0;
      new_connection->snd_nxt = 0;
      new_connection->snd_max = 0;
      new_connection->snd_wnd = 0;
      new_connection->sent_ack = 0;

      assert(relist_add(&open_tcp_connections, new_connection, NULL));
      assert(relist_add(&c->incoming_connections, new_connection, NULL));
      return new_connection;
    }
  }
  return NULL;
}

struct TcpConnection *tcp_find_connection(ipv4_t src_ip, u16 src_port,
                                          ipv4_t dst_ip, u16 dst_port) {
  for (int i = 0;; i++) {
    struct TcpConnection *c;
    int end;
    if (!relist_get(&open_tcp_connections, i, (void **)&c, &end)) {
      if (end) {
        break;
      }
      continue;
    }
    if (TCP_STATE_CLOSED == c->state) {
      continue;
    }
    if (c->incoming_port == dst_port && c->outgoing_port == src_port) {
      return c;
    }
  }
  return NULL;
}

int tcp_sync_buffer(struct TcpConnection *con);
void tcp_flush_buffers(void) {
  for (int i = 0;; i++) {
    struct TcpConnection *c;
    int end;
    if (!relist_get(&open_tcp_connections, i, (void **)&c, &end)) {
      if (end) {
        break;
      }
      continue;
    }
    if (TCP_STATE_TIME_WAIT == c->state) {
      // TODO: It should actually wait
      c->state = TCP_STATE_CLOSED;
      tcp_remove_connection(c);
      continue;
    }
    if (TCP_STATE_FIN_WAIT2 == c->state) {
      // TODO: It should actually wait
      c->state = TCP_STATE_CLOSED;
      tcp_remove_connection(c);
      continue;
    }
    if (TCP_STATE_CLOSED == c->state) {
      continue;
    }
    if (TCP_STATE_ESTABLISHED != c->state) {
      continue;
    }
    tcp_sync_buffer(c);
  }
}

void tcp_flush_acks(void) {
  for (int i = 0;; i++) {
    struct TcpConnection *c;
    int end;
    if (!relist_get(&open_tcp_connections, i, (void **)&c, &end)) {
      if (end) {
        break;
      }
      continue;
    }
    if (TCP_STATE_CLOSED == c->state) {
      continue;
    }
    if (!c->should_send_ack) {
      continue;
    }
    c->should_send_ack = 0;
    tcp_send_empty_payload(c, ACK);
  }
}

u16 tcp_find_free_port(u16 suggestion) {
  for (int i = 0;; i++) {
    struct TcpConnection *c;
    int end;
    if (!relist_get(&open_tcp_connections, i, (void **)&c, &end)) {
      if (end) {
        break;
      }
      continue;
    }
    if (c->incoming_port == suggestion) {
      suggestion++;
      // FIXME: Recursion bad
      return tcp_find_free_port(suggestion);
    }
  }
  return suggestion;
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

int tcp_sync_buffer(struct TcpConnection *con) {
  if (TCP_STATE_CLOSED == con->state) {
    return 0;
  }

  struct ringbuffer *rb = &con->outgoing_buffer;
  u32 send_buffer_len = ringbuffer_used(rb);
  if (0 == send_buffer_len) {
    return 0;
  }

  u32 len = min(tcp_can_send(con), send_buffer_len);

  char *send_buffer = kmalloc(len);
  assert(ringbuffer_read(rb, send_buffer, len) == len);
  send_tcp_packet(con, send_buffer, len);
  kfree(send_buffer);
  if (len < send_buffer_len) {
    return 0;
  } else {
    return 1;
  }
}

void tcp_strip_connection(struct TcpConnection *con) {
  ringbuffer_free(&con->incoming_buffer);
  ringbuffer_free(&con->outgoing_buffer);
}

void tcp_close(vfs_fd_t *fd) {
  struct TcpConnection *con = fd->inode->internal_object;
  if (TCP_STATE_CLOSED == con->state) {
    return;
  }
  assert(con);
  if (TCP_STATE_ESTABLISHED == con->state) {
    tcp_sync_buffer(con);
    tcp_strip_connection(con);
  }
  tcp_close_connection(con);
}

int tcp_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  struct TcpConnection *con = fd->inode->internal_object;
  assert(con);
  u32 rc = ringbuffer_read(&con->incoming_buffer, buffer, len);
  if (0 == rc && len > 0) {
    if (TCP_STATE_ESTABLISHED != con->state) {
      return -ENOTCONN;
    }
    return -EWOULDBLOCK;
  }
  return rc;
}

int tcp_has_data(vfs_inode_t *inode) {
  struct TcpConnection *con = inode->internal_object;
  if (TCP_STATE_ESTABLISHED != con->state) {
    inode->is_open = 0;
  }
  return !(ringbuffer_isempty(&con->incoming_buffer));
}

int tcp_can_write(vfs_inode_t *inode) {
  struct TcpConnection *con = inode->internal_object;
  if (TCP_STATE_ESTABLISHED != con->state) {
    inode->is_open = 0;
    return 0;
  }
  if (con->no_delay) {
    return (0 != tcp_can_send(con));
  }
  return (ringbuffer_unused(&con->outgoing_buffer) > 0) ||
         (0 != tcp_can_send(con));
}

int tcp_is_open(vfs_inode_t *inode) {
  struct TcpConnection *con = inode->internal_object;
  return (TCP_STATE_ESTABLISHED == con->state);
}

int tcp_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  (void)offset;
  struct TcpConnection *con = fd->inode->internal_object;
  assert(con);
  if (TCP_STATE_ESTABLISHED != con->state) {
    return -ENOTCONN;
  }

  if (!tcp_can_write(fd->inode)) {
    return -EWOULDBLOCK;
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
    if (!tcp_sync_buffer(con)) {
      return 0;
    }
    if (!send_tcp_packet(con, buffer, len)) {
      return -EWOULDBLOCK;
    }
  } else {
    assert(ringbuffer_write(rb, buffer, len) == len);
  }
  return len;
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
  struct TcpConnection *con = fd->inode->internal_object;

  assert(AF_INET == addr->sa_family);
  const struct sockaddr_in *in_addr = (const struct sockaddr_in *)addr;

  con->state = TCP_STATE_LISTEN;
  con->incoming_port = tcp_find_free_port(1337);
  con->outgoing_ip = in_addr->sin_addr.s_addr;
  con->outgoing_port = ntohs(in_addr->sin_port);

  con->max_seg = 1;

  con->rcv_wnd = 65535;
  con->rcv_nxt = 0;
  con->rcv_adv = 0;

  con->snd_una = 0;
  con->snd_nxt = 0;
  con->snd_max = 0;
  con->snd_wnd = 0;
  con->sent_ack = 0;

  ringbuffer_init(&con->incoming_buffer, con->rcv_wnd * 64);
  ringbuffer_init(&con->outgoing_buffer, 8192);
  con->snd_wnd = ringbuffer_unused(&con->outgoing_buffer);
  relist_add(&open_tcp_connections, con, NULL);

  con->state = TCP_STATE_SYN_SENT;
  tcp_send_empty_payload(con, SYN);

  if (!(O_NONBLOCK & fd->flags)) {
    for (;;) {
      switch_task();
      if (TCP_STATE_CLOSED == con->state) {
        return -ECONNREFUSED;
      }
      if (TCP_STATE_ESTABLISHED == con->state) {
        break;
      }
      assert(TCP_STATE_SYN_SENT == con->state ||
             TCP_STATE_ESTABLISHED == con->state);
    }
  }

  fd->inode->_has_data = tcp_has_data;
  fd->inode->_can_write = tcp_can_write;
  fd->inode->_is_open = tcp_is_open;
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

int accept(int socket, struct sockaddr *address, socklen_t *address_len) {
  (void)address;
  (void)address_len;
  vfs_fd_t *fd_ptr = get_vfs_fd(socket, NULL);
  assert(fd_ptr);
  vfs_inode_t *inode = fd_ptr->inode;
  if (OBJECT_UNIX == inode->internal_object_type) {
    SOCKET *s = (SOCKET *)inode->internal_object;
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
  if (OBJECT_TCP == inode->internal_object_type) {
    struct TcpConnection *con = inode->internal_object;

    struct TcpConnection *connection = tcp_get_incoming_connection(con, 1);
    if (!connection) {
      if (fd_ptr->flags & O_NONBLOCK) {
        return -EWOULDBLOCK;
      }
      struct pollfd fds[1];
      fds[0].fd = socket;
      fds[0].events = POLLIN;
      fds[0].revents = 0;
      for (; 1 != poll(fds, 1, 0);)
        ;
      connection = tcp_get_incoming_connection(con, 1);
    }
    assert(connection);
    vfs_inode_t *inode = vfs_create_inode(
        0 /*inode_num*/, FS_TYPE_UNIX_SOCKET, tcp_has_data, tcp_can_write, 1,
        OBJECT_TCP, connection, 0 /*file_size*/, NULL /*open*/,
        NULL /*create_file*/, tcp_read, tcp_write, tcp_close /*close*/,
        NULL /*create_directory*/, NULL /*get_vm_object*/, NULL /*truncate*/,
        NULL /*stat*/, NULL /*send_signal*/, NULL /*connect*/);
    inode->_is_open = tcp_is_open;
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

struct TcpConnection *tcp_get_incoming_connection(struct TcpConnection *con,
                                                  int remove) {
  for (int i = 0;; i++) {
    struct TcpConnection *c;
    int end;
    if (!relist_get(&con->incoming_connections, i, (void **)&c, &end)) {
      if (end) {
        break;
      }
      continue;
    }
    if (!c) {
      continue;
    }
    if (TCP_STATE_LISTEN != c->state) {
      if (remove) {
        assert(relist_remove(&con->incoming_connections, i));
      }
      return c;
    }
    // TODO: More handling with dead connections?
  }
  return NULL;
}

int tcp_has_incoming_connection(vfs_inode_t *inode) {
  struct TcpConnection *con = inode->internal_object;
  if (NULL != tcp_get_incoming_connection(con, 0)) {
    return 1;
  }
  return 0;
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
  if (OBJECT_UNIX == inode->internal_object_type) {
    SOCKET *s = (SOCKET *)inode->internal_object;
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
  } else if (OBJECT_TCP == inode->internal_object_type) {
    struct TcpConnection *con = inode->internal_object;
    // TODO: These should not be asserts
    assert(AF_INET == addr->sa_family);
    assert(addrlen == sizeof(struct sockaddr_in));
    struct sockaddr_in *in = (struct sockaddr_in *)addr;
    con->incoming_port = ntohs(in->sin_port); // TODO: Check so that it can
                                              // actually do this

    // TODO: Move this to listen()
    con->state = TCP_STATE_LISTEN;
    relist_init(&con->incoming_connections);
    fd->inode->_has_data = tcp_has_incoming_connection;
    assert(relist_add(&open_tcp_connections, con, NULL));
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

int tcp_create_fd(int is_nonblock) {
  struct TcpConnection *con = kmalloc(sizeof(struct TcpConnection));
  if (!con) {
    return -ENOMEM;
  }
  memset(con, 0, sizeof(struct TcpConnection));
  con->state = TCP_STATE_CLOSED;
  con->should_send_ack = 0;

  vfs_inode_t *inode = vfs_create_inode(
      0 /*inode_num*/, FS_TYPE_UNIX_SOCKET, NULL, always_can_write, 1,
      OBJECT_TCP, con, 0 /*file_size*/, NULL /*open*/, NULL /*create_file*/,
      tcp_read, tcp_write, NULL /*close*/, NULL /*create_directory*/,
      NULL /*get_vm_object*/, NULL /*truncate*/, NULL /*stat*/,
      NULL /*send_signal*/, tcp_connect);
  if (!inode) {
    kfree(con);
    return -ENOMEM;
  }
  inode->_is_open = tcp_is_open;
  int fd = vfs_create_fd(O_RDWR | (is_nonblock ? O_NONBLOCK : 0), 0,
                         0 /*is_tty*/, inode, NULL);
  if (fd < 0) {
    kfree(con);
    kfree(inode);
    return fd;
  }
  return fd;
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
        1 /*is_open*/, OBJECT_UNIX, new_socket /*internal_object*/,
        0 /*file_size*/, NULL /*open*/, NULL /*create_file*/, socket_read,
        socket_write, socket_close, NULL /*create_directory*/,
        NULL /*get_vm_object*/, NULL /*truncate*/, NULL /*stat*/,
        NULL /*send_signal*/, NULL /*connect*/);
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
    int is_udp = (SOCK_DGRAM & type);
    int is_nonblock = (SOCK_NONBLOCK & type);
    if (!is_udp) {
      return tcp_create_fd(is_nonblock);
    }

    vfs_inode_t *inode = vfs_create_inode(
        0 /*inode_num*/, FS_TYPE_UNIX_SOCKET, NULL, always_can_write, 1,
        OBJECT_UNIX, new_socket, 0 /*file_size*/, NULL /*open*/,
        NULL /*create_file*/, NULL, NULL, NULL /*close*/,
        NULL /*create_directory*/, NULL /*get_vm_object*/, NULL /*truncate*/,
        NULL /*stat*/, NULL /*send_signal*/, udp_connect);
    if (!inode) {
      rc = -ENOMEM;
      goto socket_error;
    }
    int n = vfs_create_fd(O_RDWR | (is_nonblock ? O_NONBLOCK : 0), 0,
                          0 /*is_tty*/, inode, NULL);
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
