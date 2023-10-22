#include <stdio.h>

int fprintf(FILE *f, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int rc = vfprintf(f, fmt, ap);
  va_end(ap);
  return rc;
}
