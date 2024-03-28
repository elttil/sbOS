#include <assert.h>
#include <errno.h>
#include <fs/devfs.h>
#include <fs/tmpfs.h>
#include <interrupts.h>
#include <network/bytes.h>
#include <network/tcp.h>
#include <poll.h>
#include <sched/scheduler.h>
#include <socket.h>

OPEN_UNIX_SOCKET *un_sockets[100] = {0};

void gen_ipv4(ipv4_t *ip, u8 i1, u8 i2, u8 i3, u8 i4) {
  ip->a[0] = i1;
  ip->a[1] = i2;
  ip->a[2] = i3;
  ip->a[3] = i4;
}

struct TcpConnection *tcp_find_connection(u16 port) {
  process_t *p = current_task;
  p = p->next;
  for (int i = 0; i < 100; i++, p = p->next) {
    if (!p) {
      p = ready_queue;
    }
    struct list *connections = &p->tcp_sockets;
    for (int i = 0;; i++) {
      struct TcpConnection *c;
      if (!list_get(connections, i, (void **)&c)) {
        break;
      }
      kprintf("got port: %d\n", c->incoming_port);
      if (c->incoming_port == port) {
        return c;
      }
    }
  }
  return NULL;
}

struct TcpListen *tcp_find_listen(u16 port) {
  process_t *p = current_task;
  process_t *s = p;
  p = p->next;
  for (; p != s; p = p->next) {
    if (!p) {
      p = ready_queue;
    }
    struct list *listen_list = &p->tcp_listen;
    for (int i = 0;; i++) {
      struct TcpListen *c;
      if (!list_get(listen_list, i, (void **)&c)) {
        break;
      }
      if (c->port == port) {
        return c;
      }
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
  struct list *connections = &current_task->tcp_sockets;
  list_add(connections, con, &connection_id);

  con->outgoing_ip = src_ip;
  con->outgoing_port = src_port;
  //  con->incoming_ip = dst_ip;
  con->incoming_port = dst_port; // FIXME: Should be different for each
                                 // connection

  con->data_file = create_fifo_object();
  stack_push(&listen->incoming_connections, (void *)connection_id);
  return con;
}

u32 tcp_listen_ipv4(u32 ip, u16 port, int *error) {
  *error = 0;

  struct TcpListen *listener = kcalloc(1, sizeof(struct TcpListen));
  listener->ip = ip;
  listener->port = port;
  stack_init(&listener->incoming_connections);

  struct list *listen_list = &current_task->tcp_listen;
  int index;
  list_add(listen_list, listener, &index);
  return index;
}

struct TcpConnection *tcp_get_connection(u32 socket, process_t *p) {
  if (!p) {
    p = current_task;
  }
  const struct list *connections = &p->tcp_sockets;
  struct TcpConnection *con;
  if (!list_get(connections, socket, (void **)&con)) {
    return NULL;
  }
  return con;
}

u32 tcp_accept(u32 listen_socket, int *error) {
  *error = 0;
  struct list *listen_list = &current_task->tcp_listen;
  struct TcpListen *l;
  if (!list_get(listen_list, listen_socket, (void **)&l)) {
    *error = 1;
    return 0;
  }
  for (;;) {
    // TODO: halt the process
    if (NULL != l->incoming_connections.head) {
      void *out = stack_pop(&l->incoming_connections);
      return (u32)out; // TODO: Should a pointer store a u32?
    }
  }
  ASSERT_NOT_REACHED;
}

u32 tcp_connect_ipv4(u32 ip, u16 port, int *error) {
  struct list *connections = &current_task->tcp_sockets;
  *error = 0;

  struct TcpConnection *con = kcalloc(1, sizeof(struct TcpConnection));
  int index;
  list_add(connections, con, &index);

  con->incoming_port = 1337; // TODO
  con->outgoing_ip = ip;
  con->outgoing_port = port;

  con->data_file = create_fifo_object();

  tcp_send_syn(con);

  for (;;) {
    tcp_wait_reply(con);
    if (con->dead) { // Port is probably closed
      *error = 1;
      return 0;
    }
    if (0 != con->handshake_state) {
      break;
    }
  }

  return index;
}

int tcp_write(u32 socket, const u8 *buffer, u64 len, u64 *out) {
  if (out) {
    *out = 0;
  }
  struct TcpConnection *con = tcp_get_connection(socket, NULL);
  if (!con) {
    return 0;
  }
  if (con->dead) {
    return 0;
  }

  send_tcp_packet(con, buffer, len);
  if (out) {
    *out = len;
  }
  return 1;
}

int tcp_read(u32 socket, u8 *buffer, u64 buffer_size, u64 *out) {
  if (out) {
    *out = 0;
  }
  struct TcpConnection *con = tcp_get_connection(socket, NULL);
  if (!con) {
    return 0;
  }
  if (con->dead) {
    return 0;
  }

  int rc = fifo_object_read(buffer, 0, buffer_size, con->data_file);
  if (rc <= 0) {
    enable_interrupts();
    rc = 0;
    return 0;
  }
  if (out) {
    *out = rc;
  }
  return 1;
}

void tcp_close(u32 socket) {
  assert(NULL);
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
  s->ptr_socket_fd->inode->has_data = 1;

  s->incoming_fd = current_task->file_descriptors[fd[1]];
  // vfs_close(fd[1]);
  return fd[0];
}

int accept(int socket, struct sockaddr *address, socklen_t *address_len) {
  (void)address;
  (void)address_len;
  vfs_inode_t *inode = current_task->file_descriptors[socket]->inode;
  SOCKET *s = (SOCKET *)inode->internal_object;

  if (NULL == s->incoming_fd) {
    // Wait until we have gotten a connection
    struct pollfd fds[1];
    fds[0].fd = socket;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    poll(fds, 1, 0);
  }

  int n = 0;
  for (; current_task->file_descriptors[n]; n++)
    ;
  current_task->file_descriptors[n] = s->incoming_fd;
  current_task->file_descriptors[n]->reference_count++;
  s->incoming_fd = NULL;
  //  for (char c; 0 < vfs_pread(s->fifo_fd, &c, 1, 0);)
  //    ;
  inode->has_data = 0;
  //  s->ptr_fifo_fd->inode->has_data = 0;

  return n;
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
    devfs_add_file(us->path, NULL, NULL, NULL, 1, 1, FS_TYPE_UNIX_SOCKET);
    return 0;
  }
  return 0;
}

int socket_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  SOCKET *s = (SOCKET *)fd->inode->internal_object;
  FIFO_FILE *file = s->fifo_file;
  int rc = fifo_object_write(buffer, 0, len, file);
  fd->inode->has_data = file->has_data;
  return rc;
}

int socket_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  SOCKET *s = (SOCKET *)fd->inode->internal_object;
  FIFO_FILE *file = s->fifo_file;
  int rc = fifo_object_read(buffer, 0, len, file);
  fd->inode->has_data = file->has_data;
  return rc;
}

void socket_close(vfs_fd_t *fd) {
  fd->inode->is_open = 0;
}

int socket(int domain, int type, int protocol) {
  if (!(AF_UNIX == domain || AF_INET == domain)) {
    return -EINVAL;
  }

  SOCKET *new_socket = kmalloc(sizeof(SOCKET));
  vfs_inode_t *inode = vfs_create_inode(
      0 /*inode_num*/, FS_TYPE_UNIX_SOCKET, 0 /*has_data*/, 1 /*can_write*/,
      1 /*is_open*/, new_socket /*internal_object*/, 0 /*file_size*/,
      NULL /*open*/, NULL /*create_file*/, socket_read, socket_write,
      socket_close, NULL /*create_directory*/, NULL /*get_vm_object*/,
      NULL /*truncate*/, NULL /*stat*/);

  vfs_fd_t *fd;
  int n = vfs_create_fd(O_RDWR | O_NONBLOCK, 0, 0 /*is_tty*/, inode, &fd);

  new_socket->domain = domain;
  new_socket->type = type;
  new_socket->protocol = protocol;
  new_socket->path = NULL;
  new_socket->incoming_fd = NULL;

  new_socket->fifo_file = create_fifo_object();

  new_socket->ptr_socket_fd = fd;
  return n;
}
