#include <stddef.h>
#include <string.h>

// FIXME: Something is weird with this function
char *strncpy(char *dest, const char *src, size_t n) {
  char *r = dest;
  for (; n && (*dest = *src); n--, src++, dest++)
    ;
  *dest = '\0';
  return r;
}
