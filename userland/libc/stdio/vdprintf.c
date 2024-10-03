#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

size_t min(size_t a, size_t b) {
  return (a < b) ? a : b;
}

int vdprintf(int fd, const char *format, va_list ap) {
  FILE f = {
      .write = write_fd,
      .fd = fd,
      .fflush = fflush_fd,
      .write_buffer = NULL,
      .read_buffer = NULL,
  };
  int rc = vfprintf(&f, format, ap);
  free(f.write_buffer);
  free(f.read_buffer);
  return rc;
}
