#include <stdio.h>

int puts(const char *s) {
  int rc = printf("%s\n", s);
  return rc;
}
