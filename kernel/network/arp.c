#include <assert.h>
#include <drivers/rtl8139.h>
#include <interrupts.h>
#include <network/arp.h>
#include <network/bytes.h>
#include <network/ethernet.h>
#include <stdio.h>
#include <string.h>

ipv4_t gateway;
ipv4_t bitmask;

void setup_network(ipv4_t _gateway, ipv4_t _bitmask) {
  gateway = _gateway;
  bitmask = _bitmask;
}

struct ARP_DATA {
  u16 htype;   // Hardware type
  u16 ptype;   // Protocol type
  u8 hlen;     // Hardware address length (Ethernet = 6)
  u8 plen;     // Protocol address length (IPv4 = 4)
  u16 opcode;  // ARP Operation Code
  u8 srchw[6]; // Source hardware address - hlen bytes (see above)
  u8 srcpr[4]; // Source protocol address - plen bytes (see above).
               // If IPv4 can just be a "u32" type.
  u8 dsthw[6]; // Destination hardware address - hlen bytes (see above)
  u8 dstpr[4]; // Destination protocol address - plen bytes (see
               // above). If IPv4 can just be a "u32" type.
};

struct ARP_TABLE_ENTRY {
  u8 is_used;
  u8 mac[6];
  u8 ip[4];
};

struct ARP_TABLE_ENTRY arp_table[10] = {0};

// FIXME: This is hardcoded, don't do this.
ipv4_t ip_address;

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

void print_mac(const char *str, u8 *mac) {
  kprintf("%s: ", str);
  for (int i = 0; i < 6; i++) {
    kprintf("%x", mac[i]);
    if (5 != i) {
      kprintf(":");
    }
  }
  kprintf("\n");
}

void print_ip(const char *str, const u8 *ip) {
  kprintf("%s: ", str);
  for (int i = 0; i < 4; i++) {
    kprintf("%d", ip[i]);
    if (3 != i) {
      kprintf(".");
    }
  }
  kprintf("\n");
}

void send_arp_request(const ipv4_t ip) {
  struct ARP_DATA data;
  data.htype = htons(1);
  data.ptype = htons(0x0800);

  data.hlen = 6;
  data.plen = 4;

  data.opcode = htons(0x0001);
  get_mac_address(data.srchw);
  memcpy(data.srcpr, &ip_address, sizeof(ipv4_t));

  memset(data.dsthw, 0, sizeof(u8[6]));
  memcpy(data.dstpr, &ip, sizeof(ipv4_t));

  u8 broadcast[6];
  memset(broadcast, 0xFF, sizeof(broadcast));
  send_ethernet_packet(broadcast, 0x0806, (u8 *)&data, sizeof(data));
}

int ip_inside_network(const ipv4_t ip) {
  if ((ip.d & bitmask.d) == (gateway.d & bitmask.d)) {
    return 1;
  }
  return 0;
}

int get_mac_from_ip(const ipv4_t ip, u8 mac[6]) {
  if (!ip_inside_network(ip)) {
    return get_mac_from_ip(gateway, mac);
  }

  for (int i = 0; i < 10; i++) {
    if (0 != memcmp(arp_table[i].ip, &ip, sizeof(u8[4]))) {
      continue;
    }
    memcpy(mac, arp_table[i].mac, sizeof(u8[6]));
    return 1;
  }
  klog(LOG_NOTE, "ARP cache miss");
  send_arp_request(ip);
  enable_interrupts();
  // TODO: Maybe wait a bit?
  for (int i = 0; i < 10; i++) {
    if (0 != memcmp(arp_table[i].ip, &ip, sizeof(u8[4]))) {
      continue;
    }
    memcpy(mac, arp_table[i].mac, sizeof(u8[6]));
    return 1;
  }
  return 0;
}

void handle_arp(const u8 *payload) {
  struct ARP_DATA *data = (struct ARP_DATA *)payload;

  // Assert that communication is over ethernet
  assert(1 == ntohs(data->htype));
  // Assert that request uses IP
  assert(0x0800 == ntohs(data->ptype));

  assert(6 == data->hlen);
  assert(4 == data->plen);
  // Assert it is a request
  if (0x0001 /*arp_request*/ == ntohs(data->opcode)) {
    struct ARP_TABLE_ENTRY *entry = find_arp_entry_to_use();
    entry->is_used = 1;
    memcpy(entry->mac, data->srchw, sizeof(u8[6]));
    memcpy(entry->ip, data->srcpr, sizeof(u8[4]));

    assert(0 == memcmp(data->dstpr, &ip_address, sizeof(u8[4])));

    // Now we have to construct a ARP response
    struct ARP_DATA response;
    response.htype = htons(1);
    response.ptype = htons(0x0800);
    response.opcode = htons(0x00002);
    response.hlen = 6;
    response.plen = 4;
    get_mac_address(response.srchw);
    memcpy(response.srcpr, &ip_address, sizeof(u8[4]));

    memcpy(response.dsthw, data->srchw, sizeof(u8[6]));
    memcpy(response.dstpr, data->srcpr, sizeof(u8[4]));

    send_ethernet_packet(data->srchw, 0x0806, (u8 *)&response,
                         sizeof(response));
  } else if (0x0002 /*arp_response*/ == ntohs(data->opcode)) {
    // Find a entry to fill
    struct ARP_TABLE_ENTRY *entry = find_arp_entry_to_use();
    entry->is_used = 1;
    memcpy(entry->mac, data->srchw, sizeof(u8[6]));
    memcpy(entry->ip, data->srcpr, sizeof(u8[4]));
  } else {
    kprintf("GOT A ARP REQEUST WITH TYPE: %x\n", ntohs(data->opcode));
    assert(0);
  }
}
