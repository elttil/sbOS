#include <stdio.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fflush.html
int fflush(FILE *stream) {
  if (stream->fflush)
    stream->fflush(stream);
  return 0;
}
