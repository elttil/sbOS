#include <stdio.h>
#include <errno.h>

extern int errno;
int remove(const char *path) {
  // FIXME
  errno = ENAMETOOLONG;
  return -1;
}
