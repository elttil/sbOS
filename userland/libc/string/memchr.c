#include <string.h>

void *memchr(const void *s, int c, size_t n) {
  const char *p = s;
  for (; n > 0; n--, p++) {
    if (*p == c) {
      return (void*)p;
    }
  }
  return NULL;
}
