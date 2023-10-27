#include <socket.h>
#include <stdint.h>

void handle_udp(uint8_t src_ip[4], const uint8_t *payload,
                uint32_t packet_length);
void send_udp_packet(struct sockaddr_in *src, const struct sockaddr_in *dst,
                     const uint8_t *payload, uint16_t payload_length);
