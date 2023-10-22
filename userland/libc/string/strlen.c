#include <string.h>

size_t strlen(const char *s) {
  const char *d = s;
  for (; *s; s++)
    ;
  return s - d;
}
