#include <syscall.h>
#include <unistd.h>
#include <errno.h>

int isatty(int fd) {
  int rc = syscall(SYS_ISATTY, fd, 0, 0, 0, 0);
  if (1 == rc)
    return rc;
  errno = -rc;
  return 0;
}
