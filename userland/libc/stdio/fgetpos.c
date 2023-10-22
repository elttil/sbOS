#include <stdio.h>

// FIXME: Error handling
int fgetpos(FILE *restrict stream, fpos_t *restrict pos) {
  *pos = (fpos_t)stream->offset_in_file;
  return 0;
}
