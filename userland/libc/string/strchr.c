#include <string.h>

char *strchr(const char *s, int c) {
  for (; *s; s++) {
    if (*s == (char)c)
      return (char*)s;
  }
  if ((char)c == '\0')
    return (char *)s;
  return NULL;
}
