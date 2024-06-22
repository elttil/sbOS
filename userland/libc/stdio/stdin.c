#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

size_t raw_write_fd(FILE *f, const unsigned char *s, size_t l) {
  int rc = write(f->fd, (char *)s, l);
  if (rc == -1) {
    f->has_error = 1;
    return 0;
  }
  return rc;
}

void fflush_fd(FILE *f) {
  raw_write_fd(f, f->write_buffer, f->write_buffer_stored);
  f->write_buffer_stored = 0;
}

size_t write_fd(FILE *f, const unsigned char *s, size_t l) {
  if (!f->write_buffer) {
    f->write_buffer = malloc(4096);
    f->write_buffer_stored = 0;
  }
  if (l > 4096) {
    return raw_write_fd(f, s, l);
  }
  if (f->write_buffer_stored + l > 4096) {
    fflush_fd(f);
  }
  memcpy(f->write_buffer + f->write_buffer_stored, s, l);
  f->write_buffer_stored += l;
  return l;
}

size_t non_cache_read_fd(FILE *f, unsigned char *s, size_t l) {
  int rc = read(f->fd, s, l);
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

    return non_cache_read_fd(f, s, l);
  }

  if (!f->read_buffer) {
    f->read_buffer = malloc(4096);
    f->read_buffer_stored = 0;
    f->read_buffer_has_read = 0;
  }
  if (f->read_buffer_stored > 0) {
    size_t read_len = min(l, f->read_buffer_stored);
    memcpy(s, f->read_buffer + f->read_buffer_has_read, read_len);
    f->read_buffer_stored -= read_len;
    f->read_buffer_has_read += read_len;
    s += read_len;
    l -= read_len;
    lseek(f->fd, read_len, SEEK_CUR);
    return read_len + read_fd(f, s, l);
  }
  if (0 == f->read_buffer_stored) {
    int offset = lseek(f->fd, 0, SEEK_CUR);
    f->read_buffer_stored = non_cache_read_fd(f, f->read_buffer, 4096);
    lseek(f->fd, offset, SEEK_SET);
    f->read_buffer_has_read = 0;
    if (0 == f->read_buffer_stored) {
      return 0;
    }
    return read_fd(f, s, l);
  }
  assert(0);
  return 0;
}

int seek_fd(FILE *stream, long offset, int whence) {
  stream->read_buffer_stored = 0;
  return lseek(stream->fd, offset, whence);
}
