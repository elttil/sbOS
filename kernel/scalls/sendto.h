#include <socket.h>
#include <typedefs.h>

struct t_two_args {
  u32 a;
  u32 b;
};
size_t syscall_sendto(int socket, const void *message, size_t length,
           int flags, struct t_two_args *extra_args /*
	   const struct sockaddr *dest_addr,
           socklen_t dest_len*/);
