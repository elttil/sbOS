#include <string.h>

char *strpbrk(const char *s1, const char *s2) {
  for (; *s1; s1++) {
    for (const char *t = s2; *t; t++) {
      if (*s1 == *t) {
        return (char *)s1;
      }
    }
  }
  return NULL;
}
