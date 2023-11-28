#include "../include/string.h"

void *memcpy(void *dest, const void *src, u32 n) {
  unsigned char *d = dest;
  const unsigned char *s = src;

  for (; n >= 8; n -= 8, d += 8, s += 8)
    *(u64 *)d = *(u64 *)s;

  for (; n >= 4; n -= 4, d += 4, s += 4)
    *(u32 *)d = *(u32 *)s;

  for (; n >= 2; n -= 2, d += 2, s += 2)
    *(u16 *)d = *(u16 *)s;

  for (; n; n--)
    *d++ = *s++;
  return dest;
}
