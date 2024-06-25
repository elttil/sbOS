#include <sys/socket.h>
#include <syscall.h>

int getpeername(int sockfd, struct sockaddr *restrict addr,
                socklen_t *restrict addrlen) {
  RC_ERRNO(syscall(SYS_GETPEERNAME, sockfd, addr, addrlen, 0, 0));
}
