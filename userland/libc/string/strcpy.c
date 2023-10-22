#include <string.h>

char *strcpy(char *dest, const char *src) {
  for (; (*dest = *src); dest++, src++)
    ;
  return dest;
}
