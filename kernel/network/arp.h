#include <typedefs.h>

extern ipv4_t ip_address;
void setup_network(ipv4_t _gateway, ipv4_t _bitmask);
int get_mac_from_ip(const ipv4_t ip, u8 mac[6]);
void send_arp_request(const ipv4_t ip);
void handle_arp(const u8 *payload);
