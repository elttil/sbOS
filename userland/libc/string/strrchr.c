#include <string.h>

char *strrchr(const char *s, int c) {
  char *last = NULL;
  for (; *s; s++) {
    if (*s == (char)c)
      last = (char *)s;
  }
  if ((char)c == '\0')
    last = (char*)s;
  return last;
}
