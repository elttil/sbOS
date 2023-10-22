#include <stdint.h>
#include <stdlib.h>

uint32_t xorshift(uint32_t x) {
  uint32_t f = x;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return f + x;
}

extern uint32_t __INTERNAL_RNG_STATE;
int rand(void) {
  uint32_t x = xorshift(__INTERNAL_RNG_STATE);
  __INTERNAL_RNG_STATE++;
  return x;
}
