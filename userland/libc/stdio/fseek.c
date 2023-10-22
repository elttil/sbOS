#include <stdio.h>
#include <assert.h>

int fseek(FILE *stream, long offset, int whence) {
    return stream->seek(stream, offset, whence);
    /*
  switch (whence) {
  case SEEK_SET:
    stream->offset_in_file = offset;
    break;
  case SEEK_CUR:
    stream->offset_in_file += offset;
    break;
  case SEEK_END:
    // FIXME
    assert(0);
    break;
  }
  // FIXME: Error checking
  return 0;*/
}
