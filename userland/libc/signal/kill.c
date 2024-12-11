#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <syscall.h>
#include <unistd.h>

int kill(int pid, int sig) {
  char buffer[4096];
  snprintf(buffer, sizeof(buffer), "/proc/%d/signal", pid);
  int fd = open(buffer, O_WRITE);
  if (-1 == fd) {
    return -1;
  }
  if (-1 == dprintf(fd, "%d", sig)) {
    return -1;
  }
  assert(-1 != close(fd));
  return 0;
}
