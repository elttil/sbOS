#include <stdio.h>

int ungetc(int c, FILE *stream) {
  if (stream->has_buffered_char)
    return EOF;
  stream->buffered_char = c;
  stream->has_buffered_char = 1;
  fseek(stream, -1, SEEK_CUR);
  return c;
}
