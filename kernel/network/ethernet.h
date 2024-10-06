#ifndef ETHERNET_H
#define ETHERNET_H
#include <typedefs.h>

struct EthernetHeader {
  u8 mac_dst[6];
  u8 mac_src[6];
  u16 type;
};

void handle_ethernet(const u8 *packet, u64 packet_length);
void send_ethernet_packet2(u8 mac_dst[6], u16 type, u64 payload_length);
void send_ethernet_packet(u8 mac_dst[6], u16 type, u8 *payload,
                          u64 payload_length);
#endif //  ETHERNET_H
