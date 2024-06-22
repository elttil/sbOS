#include <assert.h>
#include <stdio.h>

int fseek(FILE *stream, long offset, int whence) {
  stream->read_buffer_stored = 0;
  assert(stream->seek);
  return stream->seek(stream, offset, whence);
}
