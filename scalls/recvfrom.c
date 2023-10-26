#include <fs/vfs.h>
#include <poll.h>
#include <scalls/recvfrom.h>

size_t syscall_recvfrom(int socket, void *buffer, size_t length, int flags,
                        struct sockaddr *address, socklen_t *address_len) {
  if (flags & MSG_WAITALL) {
    struct pollfd fds[1];
    fds[0].fd = socket;
    fds[0].events = POLLIN;
    poll(fds, 1, 0);
  }
  kprintf("got event\n");
  return vfs_pread(socket, buffer, length, 0);
}
