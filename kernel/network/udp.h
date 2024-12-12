#include <socket.h>
#include <typedefs.h>

void handle_udp(ipv4_t src_ip, ipv4_t dst_ip, const u8 *payload,
                u32 packet_length);
void send_udp_packet(struct sockaddr_in *src, const struct sockaddr_in *dst,
                     const u8 *payload, u16 payload_length);
