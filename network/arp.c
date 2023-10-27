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

struct ARP_TABLE_ENTRY {
  uint8_t is_used;
  uint8_t mac[6];
  uint8_t ip[4];
};

struct ARP_TABLE_ENTRY arp_table[10] = {0};

// FIXME: This is hardcoded, don't do this.
uint8_t ip_address[4] = {10, 0, 2, 15};

struct ARP_TABLE_ENTRY *find_arp_entry_to_use(void) {
  // This does not need to find a "free" entry as a ARP table is
  // just a cache, it just has to pick a entry efficently.
  for (int i = 0; i < 10; i++) {
    if (!arp_table[i].is_used) {
      return &arp_table[i];
    }
  }
  return &arp_table[0];
}

void print_mac(const char *str, uint8_t *mac) {
  kprintf("%s: ", str);
  for (int i = 0; i < 6; i++) {
    kprintf("%x", mac[i]);
    if (5 != i)
      kprintf(":");
  }
  kprintf("\n");
}

void print_ip(const char *str, const uint8_t *ip) {
  kprintf("%s: ", str);
  for (int i = 0; i < 4; i++) {
    kprintf("%d", ip[i]);
    if (3 != i)
      kprintf(".");
  }
  kprintf("\n");
}

void send_arp_request(const uint8_t ip[4]) {
  struct ARP_DATA data;
  data.htype = htons(1);
  data.ptype = htons(0x0800);

  data.hlen = 6;
  data.plen = 4;

  data.opcode = htons(0x0001);
  get_mac_address(data.srchw);
  memcpy(data.srcpr, ip_address, sizeof(uint8_t[4]));

  memset(data.dsthw, 0, sizeof(uint8_t[6]));
  memcpy(data.dstpr, ip, sizeof(uint8_t[4]));

  uint8_t broadcast[6];
  memset(broadcast, 0xFF, sizeof(broadcast));
  send_ethernet_packet(broadcast, 0x0806, (uint8_t *)&data, sizeof(data));
}

int get_mac_from_ip(const uint8_t ip[4], uint8_t mac[6]) {
  print_ip("ARP GETTING MAC FROM IP: ", ip);
  for (int i = 0; i < 10; i++) {
    if (0 != memcmp(arp_table[i].ip, ip, sizeof(uint8_t[4])))
      continue;
    memcpy(mac, arp_table[i].mac, sizeof(uint8_t[6]));
    return 1;
  }
  klog("ARP cache miss", LOG_NOTE);
  asm("sti");
  send_arp_request(ip);
  // TODO: Maybe wait a bit?
  for (int i = 0; i < 10; i++) {
    if (0 != memcmp(arp_table[i].ip, ip, sizeof(uint8_t[4])))
      continue;
    memcpy(mac, arp_table[i].mac, sizeof(uint8_t[6]));
    return 1;
  }
  assert(0);
  return 0;
}

void handle_arp(const uint8_t *payload) {
  struct ARP_DATA *data = (struct ARP_DATA *)payload;

  // Assert that communication is over ethernet
  assert(1 == ntohs(data->htype));
  // Assert that request uses IP
  assert(0x0800 == ntohs(data->ptype));

  assert(6 == data->hlen);
  assert(4 == data->plen);
  // Assert it is a request
  if (0x0001 /*arp_request*/ == ntohs(data->opcode)) {
    print_mac("srchw: ", data->srchw);
    print_ip("srcpr: ", data->srcpr);

    print_mac("dsthw: ", data->dsthw);
    print_ip("dstpr: ", data->dstpr);

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

    send_ethernet_packet(data->srchw, 0x0806, (uint8_t *)&response,
                         sizeof(response));
  } else if (0x0002 /*arp_response*/ == ntohs(data->opcode)) {
    // Find a entry to fill
    struct ARP_TABLE_ENTRY *entry = find_arp_entry_to_use();
    entry->is_used = 1;
    memcpy(entry->mac, data->srchw, sizeof(uint8_t[6]));
    memcpy(entry->ip, data->srcpr, sizeof(uint8_t[4]));
    print_ip("Added ip: ", entry->ip);
  } else {
    kprintf("GOT A ARP REQEUST WITH TYPE: %x\n", ntohs(data->opcode));
    assert(0);
  }
}
