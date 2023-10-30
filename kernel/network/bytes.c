#include <network/bytes.h>

uint16_t ntohs(uint16_t net) { return (net >> 8) | (net << 8); }

uint16_t htons(uint16_t net) { return (net >> 8) | (net << 8); }

uint32_t htonl(uint32_t net) {
  return (((net & 0x000000FF) << 24) | ((net & 0x0000FF00) << 8) |
          ((net & 0x00FF0000) >> 8) | ((net & 0xFF000000) >> 24));
}
