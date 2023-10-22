#include <errno.h>
#include <fs/devfs.h>
#include <fs/tmpfs.h>
#include <poll.h>
#include <sched/scheduler.h>
#include <socket.h>

OPEN_UNIX_SOCKET *un_sockets[100] = {0};

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
  raw_vfs_pwrite(s->ptr_fifo_fd, &c, 1, 0);
  raw_vfs_pwrite(s->ptr_socket_fd, &c, 1, 0);

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
    fds[0].fd = s->fifo_fd;
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
  for (char c; 0 < vfs_pread(s->fifo_fd, &c, 1, 0);)
    ;
  inode->has_data = 0;
  //  s->ptr_fifo_fd->inode->has_data = 0;

  return n;
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  (void)addrlen;
  struct sockaddr_un *un = (struct sockaddr_un *)addr;

  vfs_fd_t *fd = get_vfs_fd(sockfd);
  if (!fd)
    return -EBADF;
  vfs_inode_t *inode = fd->inode;
  if (!inode)
    return -EBADF;
  SOCKET *s = (SOCKET *)inode->internal_object;

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
  devfs_add_file(us->path, NULL, NULL, NULL, 1, 1, FS_TYPE_UNIX_SOCKET);
  return 0;
}

int socket_write(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd) {
  (void)buffer;
  (void)offset;
  fd->inode->has_data = 1;
  return len;
}

int socket_read(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd) {
  (void)buffer;
  (void)offset;
  fd->inode->has_data = 0;
  return len;
}

void socket_close(vfs_fd_t *fd) { fd->inode->is_open = 0; }

int socket(int domain, int type, int protocol) {
  if (AF_UNIX != domain)
    return -EINVAL;

  SOCKET *new_socket = kmalloc_eternal(sizeof(SOCKET));
  vfs_inode_t *inode = vfs_create_inode(
      0 /*inode_num*/, FS_TYPE_UNIX_SOCKET, 0 /*has_data*/, 1 /*can_write*/,
      1 /*is_open*/, new_socket /*internal_object*/, 0 /*file_size*/,
      NULL /*open*/, NULL /*create_file*/, socket_read, socket_write,
      socket_close, NULL /*get_vm_object*/);

  vfs_fd_t *fd;
  int n = vfs_create_fd(O_RDWR | O_NONBLOCK, 0, inode, &fd);

  new_socket->domain = domain;
  new_socket->type = type;
  new_socket->protocol = protocol;
  new_socket->path = NULL;
  new_socket->incoming_fd = NULL;
  new_socket->fifo_fd = create_fifo();
  new_socket->ptr_fifo_fd =
      get_current_task()->file_descriptors[new_socket->fifo_fd];

  new_socket->ptr_socket_fd = fd;

  return n;
}
