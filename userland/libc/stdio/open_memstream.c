#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
/*
struct __IO_FILE {
  size_t (*write)(FILE *, const unsigned char *, size_t);
  size_t (*read)(FILE *, unsigned char *, size_t);
  int (*seek)(FILE *, long, int);
  long offset_in_file;
  int buffered_char;
  int has_buffered_char;
  int fd;
  uint8_t is_eof;
  uint8_t has_error;
  uint64_t file_size;
  void *cookie;
};
*/

struct Memstream {
  size_t buffer_usage;
  char *buffer;
};

#define MEMSTREAM_DEF_SIZE 4096

size_t memstream_write(FILE *fp, const unsigned char *buf, size_t n) {
  struct Memstream *c = fp->cookie;
  // FIXME: Do a reallocation
  if (c->buffer_usage + n >= fp->file_size) {
    n = fp->file_size - c->buffer_usage;
  }

  memcpy(c->buffer + fp->offset_in_file, buf, n);
  fp->offset_in_file += n;
  if (fp->offset_in_file > (long)c->buffer_usage)
    c->buffer_usage = fp->offset_in_file;
  return n;
}

size_t memstream_read(FILE *fp, unsigned char *buf, size_t n) {
  struct Memstream *c = fp->cookie;
  size_t length_left = c->buffer_usage - fp->offset_in_file;
  n = min(length_left, n);
  memcpy(buf, c->buffer + fp->offset_in_file, n);
  fp->offset_in_file += n;
  if (0 == n)
    fp->is_eof = 1;
  return n;
}

int memstream_seek(FILE *stream, long offset, int whence) {
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

FILE *open_memstream(char **bufp, size_t *sizep) {
  struct Memstream *c = NULL;
  FILE *fp = malloc(sizeof(FILE));
  if (!fp)
    return NULL;

  fp->offset_in_file = 0;
  fp->buffered_char = 0;
  fp->has_buffered_char = 0;
  fp->seek = memstream_seek;
  fp->fd = -1;
  fp->is_eof = 0;
  fp->has_error = 0;
  fp->file_size = MEMSTREAM_DEF_SIZE;
  fp->fflush = NULL;

  fp->can_write = 1;
  fp->can_read = 1;

  fp->write = memstream_write;
  fp->read = memstream_read;

  c = malloc(sizeof(struct Memstream));
  if (!c) {
    goto _exit_memstream_fail;
  }

  fp->cookie = (void *)c;

  c->buffer = *bufp = malloc(MEMSTREAM_DEF_SIZE);
  if (!bufp) {
    goto _exit_memstream_fail;
  }
  c->buffer_usage = 0;

  return fp;
_exit_memstream_fail:
  free(c);
  free(fp);
  return NULL;
}
