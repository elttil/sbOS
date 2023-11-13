#include <assert.h>
#include <errno.h>
#include <fs/devfs.h>
#include <fs/tmpfs.h>
#include <network/bytes.h>
#include <network/tcp.h>
#include <poll.h>
#include <sched/scheduler.h>
#include <socket.h>

// FIXME: Make these more dynamic
OPEN_UNIX_SOCKET *un_sockets[100] = {0};
OPEN_INET_SOCKET *inet_sockets[100] = {0};

struct INCOMING_TCP_CONNECTION tcp_connections[100] = {0};

int tcp_socket_write(u8 *buffer, u64 offset, u64 len,
                     vfs_fd_t *fd) {
  struct INCOMING_TCP_CONNECTION *s =
      (struct INCOMING_TCP_CONNECTION *)fd->inode->internal_object;
  if (s->connection_closed)
    return -EBADF;
  send_tcp_packet(s, buffer, len);
  return len;
}

int tcp_socket_read(u8 *buffer, u64 offset, u64 len,
                    vfs_fd_t *fd) {
  struct INCOMING_TCP_CONNECTION *s =
      (struct INCOMING_TCP_CONNECTION *)fd->inode->internal_object;
  if (s->connection_closed)
    return -EBADF;

  return fifo_object_read(buffer, offset, len, s->data_file);
}

void tcp_socket_close(vfs_fd_t *fd) {
  kprintf("TCP SOCKET CLOSE\n");
  struct INCOMING_TCP_CONNECTION *s =
      (struct INCOMING_TCP_CONNECTION *)fd->inode->internal_object;
  if (s->connection_closed) {
    s->is_used = 0;
    return;
  }
  s->requesting_connection_close = 1;
  tcp_close_connection(s);
  s->is_used = 0;
}

struct INCOMING_TCP_CONNECTION *get_incoming_tcp_connection(u8 ip[4],
                                                            u16 n_port) {
  for (int i = 0; i < 100; i++) {
    if (0 != memcmp(tcp_connections[i].ip, ip, sizeof(u8[4])))
      continue;
    if (n_port != tcp_connections[i].n_port)
      continue;
    return &tcp_connections[i];
  }
  return NULL;
}

struct INCOMING_TCP_CONNECTION *
handle_incoming_tcp_connection(u8 ip[4], u16 n_port,
                               u16 dst_port) {
  OPEN_INET_SOCKET *in = find_open_tcp_port(htons(dst_port));
  if (!in) {
    kprintf("TCP SYN to unopened port: %d\n", dst_port);
    return NULL;
  }

  int i;
  for (i = 0; i < 100; i++) {
    if (!tcp_connections[i].is_used)
      break;
  }

  tcp_connections[i].is_used = 1;
  memcpy(tcp_connections[i].ip, ip, sizeof(u8[4]));
  tcp_connections[i].n_port = n_port;
  tcp_connections[i].dst_port = dst_port;
  tcp_connections[i].data_file = create_fifo_object();

  SOCKET *s = in->s;

  vfs_inode_t *inode = vfs_create_inode(
      0 /*inode_num*/, FS_TYPE_UNIX_SOCKET, 0 /*has_data*/, 1 /*can_write*/,
      1 /*is_open*/, &tcp_connections[i], 0 /*file_size*/, NULL /*open*/,
      NULL /*create_file*/, tcp_socket_read, tcp_socket_write, tcp_socket_close,
      NULL /*create_directory*/, NULL /*get_vm_object*/, NULL /*truncate*/);

  vfs_fd_t *fd;
  int n = vfs_create_fd(O_RDWR | O_NONBLOCK, 0, inode, &fd);

  fd->reference_count++;
  s->incoming_fd = fd;

  // Shitty way of telling the accepting socket we have a incoming
  // connection.
  char c = 'i';
  fifo_object_write((u8 *)&c, 1, 0, s->fifo_file);
  s->ptr_socket_fd->inode->has_data = 1;

  vfs_close(n); // Closes the file descriptor in the current process.
                // But it does not get freed since the reference count
                // is still over zero.
  fd->reference_count--;
  kprintf("connection sent to server\n");
  return &tcp_connections[i];
}

OPEN_INET_SOCKET *find_open_tcp_port(u16 port) {
  for (int i = 0; i < 100; i++) {
    if (!inet_sockets[i])
      continue;
    kprintf("socket type: %d\n", inet_sockets[i]->s->type);
    kprintf("socket port: %d\n", inet_sockets[i]->port);
    if (inet_sockets[i]->port != port)
      continue;
    if (inet_sockets[i]->s->type != SOCK_STREAM)
      continue;
    return inet_sockets[i];
  }
  return NULL;
}

OPEN_INET_SOCKET *find_open_udp_port(u16 port) {
  for (int i = 0; i < 100; i++) {
    if (!inet_sockets[i])
      continue;
    if (inet_sockets[i]->port != port)
      continue;
    if (inet_sockets[i]->s->type != SOCK_DGRAM)
      continue;
    return inet_sockets[i];
  }
  return NULL;
}

int uds_open(const char *path) {
  // FIXME: This is super ugly

  // Find the socket that path belongs to
  SOCKET *s = NULL;
  for (int i = 0; i < 100; i++) {
    if (!un_sockets[i])
      continue;
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

  s->incoming_fd = get_current_task()->file_descriptors[fd[1]];
  // vfs_close(fd[1]);
  return fd[0];
}

int accept(int socket, struct sockaddr *address, socklen_t *address_len) {
  (void)address;
  (void)address_len;
  vfs_inode_t *inode = get_current_task()->file_descriptors[socket]->inode;
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
  for (; get_current_task()->file_descriptors[n]; n++)
    ;
  get_current_task()->file_descriptors[n] = s->incoming_fd;
  get_current_task()->file_descriptors[n]->reference_count++;
  s->incoming_fd = NULL;
  //  for (char c; 0 < vfs_pread(s->fifo_fd, &c, 1, 0);)
  //    ;
  inode->has_data = 0;
  //  s->ptr_fifo_fd->inode->has_data = 0;

  return n;
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  (void)addrlen;
  vfs_fd_t *fd = get_vfs_fd(sockfd);
  if (!fd)
    return -EBADF;
  vfs_inode_t *inode = fd->inode;
  if (!inode)
    return -EBADF;
  SOCKET *s = (SOCKET *)inode->internal_object;
  if (AF_UNIX == s->domain) {
    struct sockaddr_un *un = (struct sockaddr_un *)addr;
    size_t path_len = strlen(un->sun_path);
    s->path = kmalloc(path_len + 1);
    memcpy(s->path, un->sun_path, path_len);
    s->path[path_len] = '\0';

    OPEN_UNIX_SOCKET *us;
    int i = 0;
    for (; i < 100; i++)
      if (!un_sockets[i])
        break;

    us = un_sockets[i] = kmalloc(sizeof(OPEN_UNIX_SOCKET));

    us->path = s->path;
    us->s = s;
    s->child = us;
    devfs_add_file(us->path, NULL, NULL, NULL, 1, 1, FS_TYPE_UNIX_SOCKET);
    return 0;
  }
  if (AF_INET == s->domain) {
    struct sockaddr_in *in = (struct sockaddr_in *)addr;
    assert(in->sin_family == AF_INET); // FIXME: Figure out error value
    OPEN_INET_SOCKET *inet;
    int i = 0;
    for (; i < 100; i++)
      if (!inet_sockets[i])
        break;

    inet = inet_sockets[i] = kmalloc(sizeof(OPEN_INET_SOCKET));
    inet->address = in->sin_addr.s_addr;
    inet->port = in->sin_port;
    inet->s = s;
    s->child = inet;
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

void socket_close(vfs_fd_t *fd) { fd->inode->is_open = 0; }

int socket(int domain, int type, int protocol) {
  if (!(AF_UNIX == domain || AF_INET == domain))
    return -EINVAL;

  SOCKET *new_socket = kmalloc(sizeof(SOCKET));
  vfs_inode_t *inode = vfs_create_inode(
      0 /*inode_num*/, FS_TYPE_UNIX_SOCKET, 0 /*has_data*/, 1 /*can_write*/,
      1 /*is_open*/, new_socket /*internal_object*/, 0 /*file_size*/,
      NULL /*open*/, NULL /*create_file*/, socket_read, socket_write,
      socket_close, NULL /*create_directory*/, NULL /*get_vm_object*/,
      NULL /*truncate*/);

  vfs_fd_t *fd;
  int n = vfs_create_fd(O_RDWR | O_NONBLOCK, 0, inode, &fd);

  new_socket->domain = domain;
  new_socket->type = type;
  new_socket->protocol = protocol;
  new_socket->path = NULL;
  new_socket->incoming_fd = NULL;

  new_socket->fifo_file = create_fifo_object();

  new_socket->ptr_socket_fd = fd;
  return n;
}
