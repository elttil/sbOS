#include <stdio.h>

int ferror(FILE *stream) {
  return stream->has_error;
}
