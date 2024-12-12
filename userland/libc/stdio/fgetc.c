#include <assert.h>
#include <stdio.h>

int fgetc(FILE *stream) {
  if (stream->has_buffered_char) {
    stream->has_buffered_char = 0;
    fseek(stream, 1, SEEK_CUR);
    return stream->buffered_char;
  }
  char c;
  if (1 == fread(&c, 1, 1, stream)) {
    return (int)c;
  }
  // FIXME: Should use feof and ferror
  if (stream->has_error) {
    return EOF;
  }
  if (stream->is_eof) {
    return EOF;
  }
  // How did we get here?
  assert(0);
  return EOF;
}
