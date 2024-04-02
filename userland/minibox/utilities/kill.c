// kill
#include "include.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>

int kill_main(int argc, char **argv) {
  int fd = open_process(4);
  kill(fd, SIGTERM);
  close(fd);
  return 0;
}
