#include <stdint.h>

void handle_ipv4(const uint8_t *payload, uint32_t packet_length);
void send_ipv4_packet(uint32_t ip, uint8_t protocol, const uint8_t *payload,
                      uint16_t length);
