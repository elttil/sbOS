#include <arpa/inet.h>
#include <endian.h>
#include <stdint.h>

uint16_t ntohs(uint16_t nl) {
  return htons(nl);
}
