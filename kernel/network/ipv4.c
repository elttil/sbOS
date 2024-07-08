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

static u16 ip_checksum(const u16 *data, u16 length) {
  // Initialise the accumulator.
  u32 acc = 0xffff;

  // Handle complete 16-bit blocks.
  for (size_t i = 0; i < length / 2; i++) {
    u16 word = *(data + i);
    acc += ntohs(word);
    if (acc > 0xffff) {
      acc -= 0xffff;
    }
  }

  // Return the checksum in network byte order.
  return htons(~acc);
}

u8 ipv4_buffer[0x1000];
void send_ipv4_packet(ipv4_t ip, u8 protocol, const u8 *payload, u16 length) {
  u16 header[10];
  header[0] = (4 /*version*/ << 4) | (5 /*IHL*/);

  header[1] = htons(length + 20);

  header[2] = 0;

  header[3] = 0;
  header[4] = (protocol << 8) | 0xF8;

  header[5] = 0;
  header[6] = (ip_address.d >> 0) & 0xFFFF;
  header[7] = (ip_address.d >> 16) & 0xFFFF;

  header[8] = (ip.d >> 0) & 0xFFFF;
  header[9] = (ip.d >> 16) & 0xFFFF;

  header[5] = ip_checksum(header, 20);
  u16 packet_length = length + 20;
  assert(packet_length < sizeof(ipv4_buffer));
  u8 *packet = ipv4_buffer;
  memcpy(packet, header, 20);
  memcpy(packet + 20, payload, length);

  u8 mac[6];
  for (; !get_mac_from_ip(ip, mac);)
    ;
  send_ethernet_packet(mac, 0x0800, packet, packet_length);
}

void handle_ipv4(const u8 *payload, u32 packet_length) {
  assert(packet_length > 4);

  u16 saved_checksum = *(u16 *)(payload + 10);
  *(u16 *)(payload + 10) = 0;
  u16 calc_checksum = ip_checksum((const u16 *)payload, 20);
  *(u16 *)(payload + 10) = saved_checksum;
  if (calc_checksum != saved_checksum) {
    klog(LOG_WARN, "Invalid ipv4 checksum");
    return;
  }

  u8 version = (*payload & 0xF0) >> 4;
  u8 IHL = (*payload & 0xF);
  assert(4 == version);
  assert(5 == IHL);
  u16 ipv4_total_length = ntohs(*(u16 *)(payload + 2));
  assert(ipv4_total_length >= 20);
  // Make sure the ipv4 header is not trying to get uninitalized memory
  assert(ipv4_total_length <= packet_length);

  ipv4_t src_ip;
  memcpy(&src_ip.d, payload + 12, sizeof(u8[4]));
  ipv4_t dst_ip;
  memcpy(&dst_ip.d, payload + 16, sizeof(u8[4]));

  u8 protocol = *(payload + 9);
  switch (protocol) {
  case 0x6:
    handle_tcp(src_ip, dst_ip, payload + 20, ipv4_total_length - 20);
    break;
  case 0x11:
    handle_udp(src_ip, dst_ip, payload + 20, ipv4_total_length - 20);
    break;
  default:
    kprintf("Protocol given in IPv4 header not handeld: %x\n", protocol);
    break;
  }
}
