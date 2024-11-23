#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fopen.html
FILE *fopen(const char *pathname, const char *mode) {
  int flag = 0;
  for (; *mode; mode++) {
    // r or rb
    // Open file for reading.
    // w or wb
    // Truncate to zero length or create file for writing.
    // a or ab
    // Append; open or create file for writing at
    // end-of-file.
    switch (*mode) {
    case 'r':
      flag |= O_READ;
      break;
    case 'w':
      flag |= O_WRITE;
      break;
    case 'a':
      flag |= O_APPEND;
      break;
    }
  }

  int fd = open(pathname, flag, 0);
  if (-1 == fd) {
    return NULL;
  }

  FILE *r = fdopen(fd, mode);
  if (!r) {
    close(fd);
    return NULL;
  }
  if(flag & O_WRITE) {
    ftruncate(fd, 0);
  }
  r->has_control_over_the_fd = 1;
  return r;
}
