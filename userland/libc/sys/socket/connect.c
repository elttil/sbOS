#include <sys/socket.h>
#include <syscall.h>

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  RC_ERRNO(syscall(SYS_CONNECT, sockfd, addr, addrlen, 0, 0));
}
