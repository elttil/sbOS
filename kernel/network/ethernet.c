#include <assert.h>
#include <drivers/rtl8139.h>
#include <kmalloc.h>
#include <string.h>
#include <network/arp.h>
#include <network/bytes.h>
#include <network/ethernet.h>
#include <network/ipv4.h>
#include <stdio.h>

struct ETHERNET_HEADER {
  u8 mac_dst[6];
  u8 mac_src[6];
  u16 type;
};

u32 crc32(const char *buf, size_t len) {
  static u32 table[256];
  static int have_table = 0;
  u32 rem;
  u8 octet;
  int i, j;
  const char *p, *q;

  if (have_table == 0) {
    for (i = 0; i < 256; i++) {
      rem = i;
      for (j = 0; j < 8; j++) {
        if (rem & 1) {
          rem >>= 1;
          rem ^= 0xedb88320;
        } else {
          rem >>= 1;
        }
      }
      table[i] = rem;
    }
    have_table = 1;
  }

  u32 crc = 0xFFFFFFFF;
  q = buf + len;
  for (p = buf; p < q; p++) {
    octet = *p;
    crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
  }
  return ~crc;
}

void handle_ethernet(const u8 *packet, u64 packet_length) {
  struct ETHERNET_HEADER *eth_header = (struct ETHERNET_HEADER *)packet;
  packet += sizeof(struct ETHERNET_HEADER);
  const u8 *payload = packet;
  packet += packet_length - sizeof(struct ETHERNET_HEADER);

  u16 type = ntohs(eth_header->type);
  switch (type) {
  case 0x0806:
    handle_arp(payload);
    break;
  case 0x0800:
    handle_ipv4(payload, packet_length - sizeof(struct ETHERNET_HEADER) - 4);
    break;
  default:
    kprintf("Can't handle ethernet type 0x%x\n", type);
    break;
  }
}

void send_ethernet_packet(u8 mac_dst[6], u16 type, u8 *payload,
                          u64 payload_length) {
  assert(payload_length <= 1500);
  // FIXME: Janky allocation, do this better
  u64 buffer_size =
      sizeof(struct ETHERNET_HEADER) + payload_length + sizeof(u32);
  u8 *buffer = kmalloc(buffer_size);
  memset(buffer, 0, buffer_size);
  u8 *buffer_start = buffer;
  struct ETHERNET_HEADER *eth_header = (struct ETHERNET_HEADER *)buffer;
  buffer += sizeof(struct ETHERNET_HEADER);
  memcpy(buffer, payload, payload_length);
  buffer += payload_length;

  memcpy(eth_header->mac_dst, mac_dst, sizeof(u8[6]));
  get_mac_address(eth_header->mac_src);
  eth_header->type = htons(type);
  *(u32 *)(buffer) = htonl(crc32((const char *)buffer_start, buffer_size - 4));

  rtl8139_send_data(buffer_start, buffer_size);
  kfree(buffer_start);
}
