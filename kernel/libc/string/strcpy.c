#include <string.h>

char *strcpy(char *d, const char *s) {
  char *s1 = d;
  for (; (*d++ = *s++);)
    ;
  return s1;
}
