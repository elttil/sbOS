#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <syscall.h>

int kill_fd(int fd, int sig) {
  RC_ERRNO(syscall(SYS_KILL, fd, sig, 0, 0, 0))
}

int kill(int pid, int sig) {
  int fd = open_process(pid);
  int rc = kill_fd(fd, sig);
  close(fd);
  return rc;
}
