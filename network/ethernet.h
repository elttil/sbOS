#include <stdint.h>

void handle_ethernet(const uint8_t *packet, uint64_t packet_length);
void send_ethernet_packet(uint8_t mac_dst[6], uint16_t type, uint8_t *payload,
                          uint64_t payload_length);
