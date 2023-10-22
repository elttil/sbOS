#include <string.h>

void *memcpy(void *dest, const void *src, uint32_t n) {
  unsigned char *d = dest;
  const unsigned char *s = src;
  for (; n; n--)
    *d++ = *s++;
  return dest;
}
