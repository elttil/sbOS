#include <errno.h>
#include <stdio.h>
#include <syscall.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fflush.html
int fflush(FILE *stream) {
  if (stream) {
    if (stream->fflush) {
      stream->fflush(stream);
      return 0;
    }
  }
  errno = ENXIO;
  return -1;
}
