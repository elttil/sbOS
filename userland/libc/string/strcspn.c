#include <string.h>

size_t strcspn(const char *s1, const char *s2) {
  size_t r = 0;
  for (; *s1; s1++) {
    for (const char *t = s2; *t; t++) {
      if (*s1 == *s2) {
        r++;
        break;
      }
    }
  }
  return r;
}
