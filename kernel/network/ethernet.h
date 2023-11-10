#include <typedefs.h>

void handle_ethernet(const u8 *packet, u64 packet_length);
void send_ethernet_packet(u8 mac_dst[6], u16 type, u8 *payload,
                          u64 payload_length);
