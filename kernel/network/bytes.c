#include <network/bytes.h>

u16 ntohs(u16 net) {
  return (net >> 8) | (net << 8);
}

u16 htons(u16 net) {
  return (net >> 8) | (net << 8);
}

u32 htonl(u32 net) {
  return (((net & 0x000000FF) << 24) | ((net & 0x0000FF00) << 8) |
          ((net & 0x00FF0000) >> 8) | ((net & 0xFF000000) >> 24));
}
