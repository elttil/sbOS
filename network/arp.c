#include <assert.h>
#include <drivers/rtl8139.h>
#include <network/arp.h>
#include <network/bytes.h>
#include <network/ethernet.h>
#include <stdio.h>
#include <string.h>

struct ARP_DATA {
  uint16_t htype;   // Hardware type
  uint16_t ptype;   // Protocol type
  uint8_t hlen;     // Hardware address length (Ethernet = 6)
  uint8_t plen;     // Protocol address length (IPv4 = 4)
  uint16_t opcode;  // ARP Operation Code
  uint8_t srchw[6]; // Source hardware address - hlen bytes (see above)
  uint8_t srcpr[4]; // Source protocol address - plen bytes (see above).
                    // If IPv4 can just be a "u32" type.
  uint8_t dsthw[6]; // Destination hardware address - hlen bytes (see above)
  uint8_t dstpr[4]; // Destination protocol address - plen bytes (see
                    // above). If IPv4 can just be a "u32" type.
};

// FIXME: This is hardcoded, don't do this.
uint8_t ip_address[4] = {10, 0, 2, 15};

void print_mac(const char *str, uint8_t *mac) {
  kprintf("%s: ", str);
  for (int i = 0; i < 6; i++) {
    kprintf("%x", mac[i]);
    if (5 != i)
      kprintf(":");
  }
  kprintf("\n");
}

void print_ip(const char *str, uint8_t *ip) {
  kprintf("%s: ", str);
  for (int i = 0; i < 4; i++) {
    kprintf("%d", ip[i]);
    if (3 != i)
      kprintf(".");
  }
  kprintf("\n");
}

void handle_arp(uint8_t *payload) {
  struct ARP_DATA *data = (struct ARP_DATA *)payload;

  // Assert that communication is over ethernet
  assert(1 == ntohs(data->htype));
  // Assert it is a request
  assert(0x0001 == ntohs(data->opcode));
  // Assert that request uses IP
  assert(0x0800 == ntohs(data->ptype));

  assert(6 == data->hlen);
  assert(4 == data->plen);
  print_mac("srchw: ", data->srchw);
  print_ip("srcpr: ", data->srcpr);

  print_mac("dsthw: ", data->dsthw);
  print_ip("dstpr: ", data->dstpr);

  uint8_t mac[6];
  get_mac_address(mac);
  print_mac("THIS DEVICE MAC: ", mac);
  assert(0 == memcmp(data->dstpr, ip_address, sizeof(uint8_t[4])));

  // Now we have to construct a ARP response
  struct ARP_DATA response;
  response.htype = htons(1);
  response.ptype = htons(0x0800);
  response.opcode = htons(0x00002);
  response.hlen = 6;
  response.plen = 4;
  get_mac_address(response.srchw);
  memcpy(response.srcpr, ip_address, sizeof(uint8_t[4]));

  memcpy(response.dsthw, data->srchw, sizeof(uint8_t[6]));
  memcpy(response.dstpr, data->srcpr, sizeof(uint8_t[4]));

  send_ethernet_packet(data->srchw, 0x0806, (uint8_t*)&response, sizeof(response));
}
