#include "include.h"
#include <fcntl.h>

int touch_main(int argc, char **argv) {
  if (argc < 2)
    return 1;
  int fd;
  COND_PERROR_EXP(open(argv[1], 0, O_CREAT), "open", return 1)
  close(fd);
  return 0;
}
