#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fclose.html
// FIXME: Do some actual error checking.
int fclose(FILE *stream) {
  if (stream) {
    if (stream->fflush) {
      stream->fflush(stream);
    }
    close(stream->fd);
    free(stream->cookie);
  }
  free(stream);
  return 0;
}
