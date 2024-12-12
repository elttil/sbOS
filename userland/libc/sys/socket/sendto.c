#include <sys/socket.h>
#include <syscall.h>

struct two_args {
  uint32_t a;
  uint32_t b;
};

size_t sendto(int socket, const void *message, size_t length, int flags,
              const struct sockaddr *dest_addr, socklen_t dest_len) {
  struct two_args extra = {
      .a = dest_addr,
      .b = dest_len,
  };
  return syscall(SYS_SENDTO, socket, message, length, flags, &extra);
}
