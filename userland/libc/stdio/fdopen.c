#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

// FIXME: All modes not implemented
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fdopen.html
FILE *fdopen(int fildes, const char *mode) {
  FILE *r = calloc(1, sizeof(FILE));
  if (!r) {
    errno = -ENOMEM;
    return NULL;
  }

  for (; *mode; mode++) {
    // r or rb
    // Open file for reading.
    // w or wb
    // Truncate to zero length or create file for writing.
    // a or ab
    // Append; open or create file for writing at
    // end-of-file.
    switch (*mode) {
    case 'r':
      r->can_read = 1;
      break;
    case 'w':
      r->can_write = 1;
      break;
    case 'a':
      r->append = 1;
      break;
    }
  }

  struct stat s;
  if (-1 == fstat(fildes, &s)) {
    free(r);
    errno = -EBADF;
    return NULL;
  }

  r->read = read_fd;
  r->write = write_fd;
  r->seek = seek_fd;
  r->has_error = 0;
  r->is_eof = 0;
  r->offset_in_file = 0;
  r->file_size = s.st_size;
  r->cookie = NULL;
  r->fd = fildes;
  r->read_buffer = NULL;
  r->read_buffer_stored = 0;
  r->fflush = fflush_fd;
  r->has_control_over_the_fd = 0;
  return r;
}
