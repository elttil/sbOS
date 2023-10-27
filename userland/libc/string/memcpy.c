#include <string.h>

void *memcpy(void *dest, const void *src, uint32_t n) {
  unsigned char *d = dest;
  const unsigned char *s = src;

  for (; n >= 8; n -= 8, d += 8, s += 8)
    *(uint64_t *)d = *(uint64_t *)s;

  for (; n >= 4; n -= 4, d += 4, s += 4)
    *(uint32_t *)d = *(uint32_t *)s;

  for (; n >= 2; n -= 2, d += 2, s += 2)
    *(uint16_t *)d = *(uint16_t *)s;

  for (; n; n--)
    *d++ = *s++;
  return dest;
}
