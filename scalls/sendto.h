#include <socket.h>
#include <stdint.h>

struct t_two_args {
  uint32_t a;
  uint32_t b;
};
size_t syscall_sendto(int socket, const void *message, size_t length,
           int flags, struct t_two_args *extra_args /*
	   const struct sockaddr *dest_addr,
           socklen_t dest_len*/);
