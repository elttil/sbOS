#include <assert.h>
#include <drivers/pit.h>
#include <kmalloc.h>
#include <network/arp.h>
#include <network/bytes.h>
#include <network/ethernet.h>
#include <network/ipv4.h>
#include <network/tcp.h>
#include <network/udp.h>
#include <string.h>

uint16_t ip_checksum(void *vdata, size_t length) {
  // Cast the data pointer to one that can be indexed.
  char *data = (char *)vdata;

  // Initialise the accumulator.
  uint32_t acc = 0xffff;

  // Handle complete 16-bit blocks.
  for (size_t i = 0; i + 1 < length; i += 2) {
    uint16_t word;
    memcpy(&word, data + i, 2);
    acc += ntohs(word);
    if (acc > 0xffff) {
      acc -= 0xffff;
    }
  }

  // Handle any partial block at the end of the data.
  if (length & 1) {
    uint16_t word = 0;
    memcpy(&word, data + length - 1, 1);
    acc += ntohs(word);
    if (acc > 0xffff) {
      acc -= 0xffff;
    }
  }

  // Return the checksum in network byte order.
  return htons(~acc);
}

extern uint8_t ip_address[4];
void send_ipv4_packet(uint32_t ip, uint8_t protocol, const uint8_t *payload,
                      uint16_t length) {
  uint8_t header[20] = {0};
  header[0] = (4 /*version*/ << 4) | (5 /*IHL*/);
  *((uint16_t *)(header + 2)) = htons(length + 20);
  header[8 /*TTL*/] = 0xF8;
  header[9] = protocol;

  memcpy(header + 12 /*src_ip*/, ip_address, sizeof(uint8_t[4]));
  memcpy(header + 16, &ip, sizeof(uint8_t[4]));

  *((uint16_t *)(header + 10 /*checksum*/)) = ip_checksum(header, 20);
  uint16_t packet_length = length + 20;
  uint8_t *packet = kmalloc(packet_length);
  memcpy(packet, header, 20);
  memcpy(packet + 20, payload, length);

  uint8_t mac[6];
  uint8_t ip_copy[4]; // TODO: Do I need to do this?
  memcpy(ip_copy, &ip, sizeof(uint8_t[4]));
  for (; !get_mac_from_ip(ip_copy, mac);)
    ;
  kprintf("pre send_ethernet: %x\n", pit_num_ms());
  send_ethernet_packet(mac, 0x0800, packet, packet_length);
  kprintf("after send_ethernet: %x\n", pit_num_ms());
  kfree(packet);
}

void handle_ipv4(const uint8_t *payload, uint32_t packet_length) {
  assert(packet_length > 4);

  uint16_t saved_checksum = *(uint16_t *)(payload + 10);
  *(uint16_t *)(payload + 10) = 0;
  uint16_t calc_checksum = ip_checksum((uint8_t *)payload, 20);
  *(uint16_t *)(payload + 10) = saved_checksum;
  assert(calc_checksum == saved_checksum);

  uint8_t version = (*payload & 0xF0) >> 4;
  uint8_t IHL = (*payload & 0xF);
  kprintf("version: %x\n", version);
  assert(4 == version);
  assert(5 == IHL);
  uint16_t ipv4_total_length = ntohs(*(uint16_t *)(payload + 2));
  assert(ipv4_total_length >= 20);
  // Make sure the ipv4 header is not trying to get uninitalized memory
  assert(ipv4_total_length <= packet_length);

  uint8_t src_ip[4];
  memcpy(src_ip, payload + 12, sizeof(uint8_t[4]));

  uint8_t protocol = *(payload + 9);
  switch (protocol) {
  case 0x6:
    handle_tcp(src_ip, payload + 20, ipv4_total_length - 20);
    break;
  case 0x11:
    handle_udp(src_ip, payload + 20, ipv4_total_length - 20);
    break;
  default:
    kprintf("Protocol given in IPv4 header not handeld: %x\n", protocol);
    break;
  }
}
