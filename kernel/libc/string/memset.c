#include <string.h>

void *memset(void *dst, const unsigned char c, u32 n) {
  uintptr_t d = (uintptr_t)dst;
  for (u32 i = 0; i < n; i++, d++)
    *(unsigned char *)d = c;

  return (void *)d;
}
