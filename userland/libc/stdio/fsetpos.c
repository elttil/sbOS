#include <stdio.h>

// FIXME: Error handling
int fsetpos(FILE *stream, const fpos_t *pos) {
  stream->offset_in_file = (long)(*pos);
  return 0;
}
