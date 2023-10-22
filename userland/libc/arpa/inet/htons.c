#include <arpa/inet.h>
#include <endian.h>
#include <stdint.h>

uint16_t htons(uint16_t hostlong) {
#if BYTE_ORDER == LITTLE_ENDIAN
  hostlong = ((hostlong & 0xFF00) >> 8) | ((hostlong & 0x00FF) << 8);
#endif
  return hostlong;
}
