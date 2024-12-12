#include <string.h>

char *strstr(const char *s1, const char *s2) {
  // If s2 points to a string with zero length, the function shall return s1.
  if ('\0' == *s2) {
    return (char *)s1;
  }
  for (; *s1; s1++) {
    const char *t1 = s1;
    const char *t2 = s2;
    int is_dif = 0;
    for (; *t2 && *t1; t1++, t2++) {
      if (*t2 != *t1) {
        is_dif = 1;
        break;
      }
    }
    if (!is_dif) {
      return (char *)s1;
    }
  }
  return NULL;
}
