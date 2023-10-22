#include <assert.h>
#include <stdio.h>
#include <unistd.h>

size_t write_fd(FILE *f, const unsigned char *s, size_t l) {
  int rc = pwrite(f->fd, s, l, f->offset_in_file);
  if (rc == -1) {
    f->has_error = 1;
    return 0;
  }
  f->offset_in_file += rc;
  return rc;
}

size_t read_fd(FILE *f, unsigned char *s, size_t l) {
  int rc = pread(f->fd, s, l, f->offset_in_file);
  if (rc == 0)
    f->is_eof = 1;
  if (rc == -1) {
    f->has_error = 1;
    return 0;
  }
  f->offset_in_file += rc;
  return rc;
}

int seek_fd(FILE *stream, long offset, int whence) {
  switch (whence) {
  case SEEK_SET:
    stream->offset_in_file = offset;
    break;
  case SEEK_CUR:
    stream->offset_in_file += offset;
    break;
  case SEEK_END:
    stream->offset_in_file = stream->file_size + offset;
    break;
  default:
    assert(0);
    break;
  }
  // FIXME: Error checking
  return 0;
}

FILE __stdin_FILE = {
    .write = write_fd,
    .read = read_fd,
    .seek = NULL,
    .is_eof = 0,
    .has_error = 0,
    .cookie = NULL,
    .fd = 0,
};
