#include "../include/string.h"

void *__attribute__((optimize("O0")))
memcpy(void *dest, const void *src, uint32_t n) {
  unsigned char *d = dest;
  const unsigned char *s = src;
  //	for(;n--;) *d++ = *s++;
  for (; n; n--)
    *d++ = *s++;
  return dest;
}
