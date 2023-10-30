#include <assert.h>
#include <fs/vfs.h>
#include <math.h>
#include <poll.h>
#include <scalls/recvfrom.h>

size_t syscall_recvfrom(
    int socket, void *buffer, size_t length, int flags,
    struct two_args
        *extra_args /*struct sockaddr *address, socklen_t *address_len*/) {

  struct sockaddr *address = (struct sockaddr *)extra_args->a;
  socklen_t *address_len = (socklen_t *)extra_args->b;
  kprintf("address: %x\n", address);
  kprintf("address_len: %x\n", address_len);

  if (flags & MSG_WAITALL) {
    struct pollfd fds[1];
    fds[0].fd = socket;
    fds[0].events = POLLIN;
    poll(fds, 1, 0);
  }

  uint16_t data_length;
  socklen_t tmp_socklen;
  vfs_pread(socket, &tmp_socklen, sizeof(socklen_t), 0);
  if (address_len)
    *address_len = tmp_socklen;
  if (address) {
    vfs_pread(socket, address, tmp_socklen, 0);
  } else {
    // We still have to throwaway the data.
    char devnull[100];
    for (; tmp_socklen;) {
      int rc = vfs_pread(socket, devnull, min(tmp_socklen, 100), 0);
      assert(rc >= 0);
      tmp_socklen -= rc;
    }
  }

  vfs_pread(socket, &data_length, sizeof(data_length), 0);
  // If it is reading less than the packet length that could cause
  // problems as the next read will not be put at a new header. Luckily
  // it seems as if other UNIX systems can discard the rest of the
  // packet if not read.

  // Read in the data requested
  int read_len = min(length, data_length);
  int rc = vfs_pread(socket, buffer, read_len, 0);
  // Discard the rest of the packet
  int rest = data_length - read_len;
  char devnull[100];
  for (; rest;) {
    int rc = vfs_pread(socket, devnull, 100, 0);
    assert(rc >= 0);
    rest -= rc;
  }
  return rc;
}
