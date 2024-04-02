#include <signal.h>
#include <syscall.h>

int kill(int fd, int sig) {
  RC_ERRNO(syscall(SYS_KILL, fd, sig, 0, 0, 0))
}
