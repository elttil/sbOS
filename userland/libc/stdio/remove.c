#include <stdio.h>
#include <errno.h>

extern int errno;
int remove(const char *path) {
	(void)path;
  // FIXME
  errno = ENAMETOOLONG;
  return -1;
}
