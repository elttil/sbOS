#include <arpa/inet.h>
#include <endian.h>
#include <stdint.h>

uint32_t htonl(uint32_t hostlong) {
#if BYTE_ORDER == LITTLE_ENDIAN
  hostlong = (uint32_t)(htons(hostlong >> 16)) |
             (((uint32_t)htons(hostlong & 0xFFFF)) << 16);
#endif
  return hostlong;
}
