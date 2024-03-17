#include <assert.h>
#include <network/bytes.h>
#include <network/ipv4.h>
#include <network/udp.h>
#include <socket.h>

void send_udp_packet(struct sockaddr_in *src, const struct sockaddr_in *dst,
                     const u8 *payload, u16 payload_length) {
  u16 header[4] = {0};
  header[0] = src->sin_port;
  header[1] = dst->sin_port;
  header[2] = htons(payload_length + 8);

  u16 packet_length = sizeof(header) + payload_length;
  u8 *packet = kmalloc(packet_length);
  memcpy(packet, header, sizeof(header));
  memcpy(packet + sizeof(header), payload, payload_length);
  send_ipv4_packet((ipv4_t){.d = dst->sin_addr.s_addr}, 0x11, packet, packet_length);
  kfree(packet);
}

void handle_udp(ipv4_t src_ip, const u8 *payload, u32 packet_length) {
  // TODO: Reimplement
  assert(NULL);
}
