#include <stdio.h>

long ftell(FILE *stream) {
  return fseek(stream, 0, SEEK_CUR);
}
