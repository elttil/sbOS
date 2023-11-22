#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

size_t min(size_t a, size_t b) { return (a < b) ? a : b; }

int vdprintf(int fd, const char *format, va_list ap) {
  FILE f = {
      .write = write_fd,
      .fd = fd,
      .fflush = fflush_fd,
      .write_buffer = NULL,
  };
  return vfprintf(&f, format, ap);
}
