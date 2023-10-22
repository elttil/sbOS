#include <stdint.h>
#include <stdlib.h>

uint32_t __INTERNAL_RNG_STATE;
void srand(unsigned int seed) {
  __INTERNAL_RNG_STATE = seed;
  __INTERNAL_RNG_STATE = rand(); // rand() used the internal rng state
}
