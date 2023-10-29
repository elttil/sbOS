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

size_t non_cache_read_fd(FILE *f, unsigned char *s, size_t l) {
  int rc = pread(f->fd, s, l, f->offset_in_file);
  if (rc == 0)
    f->is_eof = 1;
  if (rc == -1) {
    f->has_error = 1;
    return 0;
  }
  return rc;
}

size_t read_fd(FILE *f, unsigned char *s, size_t l) {
  if (0 == l)
    return 0;

  // Skip using cache if the length being requested if longer than or
  // equal to the cache block size. This avoids doing a bunch of extra
  // syscalls
  if (l >= 4096) {
    // Invalidate the cache
f->read_buffer_stored = 0;

    size_t rc = non_cache_read_fd(f, s, l);
    f->offset_in_file += rc;
    return rc;
  }

  if (!f->read_buffer) {
    f->read_buffer = malloc(4096);
    f->read_buffer_stored = 0;
    f->read_buffer_has_read = 0;
  }
  if (f->read_buffer_stored > 0) {
    size_t read_len = min(l, f->read_buffer_stored);
    f->offset_in_file += read_len;
    memcpy(s, f->read_buffer + f->read_buffer_has_read, read_len);
    f->read_buffer_stored -= read_len;
    f->read_buffer_has_read += read_len;
    s += read_len;
    l -= read_len;
    return read_len + read_fd(f, s, l);
  }
  if (0 == f->read_buffer_stored) {
    f->read_buffer_stored = non_cache_read_fd(f, f->read_buffer, 4096);
    f->read_buffer_has_read = 0;
    if (0 == f->read_buffer_stored) {
      return 0;
    }
    return read_fd(f, s, l);
  }
  assert(0);
}

int seek_fd(FILE *stream, long offset, int whence) {
  stream->read_buffer_stored = 0;
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
