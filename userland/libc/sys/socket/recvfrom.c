#include <assert.h>
#include <sys/socket.h>
#include <syscall.h>

struct two_args {
  uint32_t a;
  uint32_t b;
};

size_t recvfrom(int socket, void *buffer, size_t length, int flags,
                struct sockaddr *address, socklen_t *address_len) {
  struct two_args extra_args;
  extra_args.a = address;
  extra_args.b = address_len;

  return syscall(SYS_RECVFROM, socket, buffer, length, flags, &extra_args);
}
