#include <stdio.h>
#include <stdlib.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fclose.html
// FIXME: Do some actual error checking.
int fclose(FILE *stream) {
  if (stream)
    free(stream->cookie);
  free(stream);
  return 0;
}
