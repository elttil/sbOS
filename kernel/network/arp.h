#include <typedefs.h>

void setup_network(ipv4_t _gateway, ipv4_t _bitmask);
int get_mac_from_ip(const u8 ip[4], u8 mac[6]);
void send_arp_request(const u8 ip[4]);
void handle_arp(const u8 *payload);
