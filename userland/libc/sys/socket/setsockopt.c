#include <sys/socket.h>
#include <syscall.h>

int setsockopt(int socket, int level, int option_name, const void *option_value,
               socklen_t option_len) {
  RC_ERRNO(syscall(SYS_SETSOCKOPT, socket, level, option_name, option_value,
                   option_len));
}
