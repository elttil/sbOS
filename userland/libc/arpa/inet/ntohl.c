#include <arpa/inet.h>
#include <endian.h>
#include <stdint.h>

uint32_t ntohl(uint32_t nl) {
  return htonl(nl);
}
