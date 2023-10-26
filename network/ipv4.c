#include <assert.h>
#include <network/bytes.h>
#include <network/ipv4.h>
#include <network/udp.h>

void handle_ipv4(const uint8_t *payload, uint32_t packet_length) {
  assert(packet_length > 4);
  uint8_t version = (*payload & 0xF0) >> 4;
  uint8_t IHL = (*payload & 0xF);
  kprintf("version: %x\n", version);
  assert(4 == version);
  assert(5 == IHL);
  uint16_t ipv4_total_length = ntohs(*(uint16_t *)(payload + 2));
  assert(ipv4_total_length >= 20);
  // Make sure the ipv4 header is not trying to get uninitalized memory
  assert(ipv4_total_length <= packet_length);

  uint8_t protocol = *(payload + 9);
  switch (protocol) {
  case 0x11:
    handle_udp(payload + 20, ipv4_total_length - 20);
    break;
  default:
    kprintf("Protocol given in IPv4 header not handeld: %x\n", protocol);
    break;
  }
}
