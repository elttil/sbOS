#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>

// FIXME: All modes not implemented
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fopen.html
FILE *fopen(const char *pathname, const char *mode) {
  uint8_t read = 0;
  uint8_t write = 0;
  uint8_t append = 0;
  // FIXME: Not parsed correctly
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
      read = 1;
      break;
    case 'w':
      write = 1;
      break;
    case 'a':
      append = 1;
      break;
    }
  }
  int flag = 0;
  if (read)
    flag |= O_READ;
  if (write)
    flag |= O_WRITE;

  int fd = open(pathname, flag, 0);
  if (-1 == fd)
    return NULL;

  struct stat s;
  stat(pathname, &s);

  FILE *r = malloc(sizeof(FILE));
  r->read = read_fd;
  r->write = write_fd;
  r->seek = seek_fd;
  r->has_error = 0;
  r->is_eof = 0;
  r->offset_in_file = 0;
  r->file_size = s.st_size;
  r->cookie = NULL;
  r->fd = fd;
  return r;
}
