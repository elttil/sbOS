#include <errno.h>
#include <stdio.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fileno.html
// The fileno() function shall return the integer file descriptor associated
// with the stream pointed to by stream.
int fileno(FILE *stream) {
  if (-1 == stream->fd) {
    errno = EBADF;
    return -1;
  }
  return stream->fd;
}
