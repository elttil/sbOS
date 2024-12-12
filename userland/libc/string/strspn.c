#include <string.h>

size_t strspn(const char *s1, const char *s2) {
  size_t r = 0;
  for (; *s1; s1++) {
    int e = 0;
    for (const char *t = s2; *t; t++) {
      if (*s1 == *t) {
        e = 1;
        break;
      }
    }
    if (!e) {
      break;
    }
    r++;
  }
  return r;
}
