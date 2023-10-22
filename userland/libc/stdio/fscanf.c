#include <stdio.h>
#include <assert.h>

int fscanf(FILE *stream, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int rc = vfscanf(stream, format, ap);
  va_end(ap);
  return rc;
}
