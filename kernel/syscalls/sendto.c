#include <assert.h>
#include <network/bytes.h>
#include <network/udp.h>
#include <syscalls.h>

size_t syscall_sendto(int socket, const void *message, size_t length,
           int flags, struct t_two_args *extra_args /*
	   const struct sockaddr *dest_addr,
           socklen_t dest_len*/) {
  const struct sockaddr *dest_addr = (const struct sockaddr *)extra_args->a;
  socklen_t dest_len = (socklen_t)extra_args->b;
  (void)dest_len;
  vfs_fd_t *fd = get_vfs_fd(socket);
  assert(fd);
  SOCKET *s = (SOCKET *)fd->inode->internal_object;
  OPEN_INET_SOCKET *inet = s->child;
  assert(inet);
  struct sockaddr_in in;
  in.sin_addr.s_addr = inet->address;
  in.sin_port = inet->port;
  send_udp_packet(&in, (const struct sockaddr_in *)dest_addr, message, length);
  return length; // FIXME: This is probably not true.
}
