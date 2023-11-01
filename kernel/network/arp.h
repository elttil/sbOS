#include <stdint.h>

int get_mac_from_ip(const uint8_t ip[4], uint8_t mac[6]);
void send_arp_request(const uint8_t ip[4]);
void handle_arp(const uint8_t *payload);
