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

u16 ip_checksum(void *vdata, size_t length) {
  // Cast the data pointer to one that can be indexed.
  char *data = (char *)vdata;

  // Initialise the accumulator.
  u32 acc = 0xffff;

  // Handle complete 16-bit blocks.
  for (size_t i = 0; i + 1 < length; i += 2) {
    u16 word;
    memcpy(&word, data + i, 2);
    acc += ntohs(word);
    if (acc > 0xffff) {
      acc -= 0xffff;
    }
  }

  // Handle any partial block at the end of the data.
  if (length & 1) {
    u16 word = 0;
    memcpy(&word, data + length - 1, 1);
    acc += ntohs(word);
    if (acc > 0xffff) {
      acc -= 0xffff;
    }
  }

  // Return the checksum in network byte order.
  return htons(~acc);
}

extern u8 ip_address[4];
void send_ipv4_packet(u32 ip, u8 protocol, const u8 *payload, u16 length) {
  u8 header[20] = {0};
  header[0] = (4 /*version*/ << 4) | (5 /*IHL*/);
  *((u16 *)(header + 2)) = htons(length + 20);
  header[8 /*TTL*/] = 0xF8;
  header[9] = protocol;

  memcpy(header + 12 /*src_ip*/, ip_address, sizeof(u8[4]));
  memcpy(header + 16, &ip, sizeof(u8[4]));

  *((u16 *)(header + 10 /*checksum*/)) = ip_checksum(header, 20);
  u16 packet_length = length + 20;
  u8 *packet = kmalloc(packet_length);
  memcpy(packet, header, 20);
  memcpy(packet + 20, payload, length);

  u8 mac[6];
  u8 ip_copy[4]; // TODO: Do I need to do this?
  memcpy(ip_copy, &ip, sizeof(u8[4]));
  for (; !get_mac_from_ip(ip_copy, mac);)
    ;
  send_ethernet_packet(mac, 0x0800, packet, packet_length);
  kfree(packet);
}

void handle_ipv4(const u8 *payload, u32 packet_length) {
  assert(packet_length > 4);

  u16 saved_checksum = *(u16 *)(payload + 10);
  *(u16 *)(payload + 10) = 0;
  u16 calc_checksum = ip_checksum((u8 *)payload, 20);
  *(u16 *)(payload + 10) = saved_checksum;
  if(calc_checksum != saved_checksum)
	  return;

  u8 version = (*payload & 0xF0) >> 4;
  u8 IHL = (*payload & 0xF);
  assert(4 == version);
  assert(5 == IHL);
  u16 ipv4_total_length = ntohs(*(u16 *)(payload + 2));
  assert(ipv4_total_length >= 20);
  // Make sure the ipv4 header is not trying to get uninitalized memory
  assert(ipv4_total_length <= packet_length);

  u8 src_ip[4];
  memcpy(src_ip, payload + 12, sizeof(u8[4]));

  u8 protocol = *(payload + 9);
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
