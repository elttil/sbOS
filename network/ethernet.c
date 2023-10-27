#include <assert.h>
#include <drivers/rtl8139.h>
#include <kmalloc.h>
#include <network/arp.h>
#include <network/bytes.h>
#include <network/ethernet.h>
#include <network/ipv4.h>
#include <stdio.h>

struct ETHERNET_HEADER {
  uint8_t mac_dst[6];
  uint8_t mac_src[6];
  uint16_t type;
};

uint32_t crc32(const char *buf, size_t len) {
  static uint32_t table[256];
  static int have_table = 0;
  uint32_t rem;
  uint8_t octet;
  int i, j;
  const char *p, *q;

  if (have_table == 0) {
    for (i = 0; i < 256; i++) {
      rem = i;
      for (j = 0; j < 8; j++) {
        if (rem & 1) {
          rem >>= 1;
          rem ^= 0xedb88320;
        } else
          rem >>= 1;
      }
      table[i] = rem;
    }
    have_table = 1;
  }

  uint32_t crc = 0xFFFFFFFF;
  q = buf + len;
  for (p = buf; p < q; p++) {
    octet = *p;
    crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
  }
  return ~crc;
}

void handle_ethernet(const uint8_t *packet, uint64_t packet_length) {
  struct ETHERNET_HEADER *eth_header = (struct ETHERNET_HEADER *)packet;
  packet += sizeof(struct ETHERNET_HEADER);
  const uint8_t *payload = packet;
  packet += packet_length - sizeof(struct ETHERNET_HEADER);
  uint32_t crc = *((uint32_t *)packet - 1);
  kprintf("PACKET crc: %x\n", crc);
  kprintf("OUR OWN CALCULATED crc: %x\n",
          crc32((const char *)eth_header, (packet_length - 4)));

  uint16_t type = ntohs(eth_header->type);
  switch (type) {
  case 0x0806:
    handle_arp(payload);
    break;
  case 0x0800:
    handle_ipv4(payload, packet_length - sizeof(struct ETHERNET_HEADER) - 4);
    break;
  default:
    kprintf("Can't handle ethernet type\n");
    break;
  }
}

void send_ethernet_packet(uint8_t mac_dst[6], uint16_t type, uint8_t *payload,
                          uint64_t payload_length) {
  // FIXME: Janky allocation, do this better
  uint64_t buffer_size =
      sizeof(struct ETHERNET_HEADER) + payload_length + sizeof(uint32_t);
  uint8_t *buffer = kmalloc(buffer_size);
  uint8_t *buffer_start = buffer;
  struct ETHERNET_HEADER *eth_header = (struct ETHERNET_HEADER *)buffer;
  buffer += sizeof(struct ETHERNET_HEADER);
  memcpy(buffer, payload, payload_length);
  buffer += payload_length;

  memcpy(eth_header->mac_dst, mac_dst, sizeof(uint8_t[6]));
  get_mac_address(eth_header->mac_src);
  eth_header->type = htons(type);
  *(uint32_t *)(buffer) =
      htonl(crc32((const char *)buffer_start, buffer_size - 4));

  assert(rtl8139_send_data(buffer_start, buffer_size));
  kfree(buffer_start);
  kprintf("sent data\n");
}
