#include <typedefs.h>

void handle_ipv4(const u8 *payload, u32 packet_length);
void send_ipv4_packet(ipv4_t ip, u8 protocol, const u8 *payload, u16 length);
void send_ipv4_packet2(ipv4_t ip, u8 protocol, u16 length);
