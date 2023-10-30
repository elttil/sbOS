#include <string.h>

void *memset(void *dst, const unsigned char c, uint32_t n) {
  uintptr_t d = (uintptr_t)dst;
  for (uint32_t i = 0; i < n; i++, d++)
    *(unsigned char *)d = c;

  return (void *)d;
}
