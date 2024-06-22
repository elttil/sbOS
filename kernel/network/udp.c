#include <assert.h>
#include <network/bytes.h>
#include <network/ipv4.h>
#include <network/udp.h>
#include <socket.h>

void send_udp_packet(struct sockaddr_in *src, const struct sockaddr_in *dst,
                     const u8 *payload, u16 payload_length) {
  assert(src);

  assert(dst);

  u16 header[4] = {0};
  header[0] = src->sin_port;
  header[1] = dst->sin_port;
  header[2] = htons(payload_length + 8);

  u16 packet_length = sizeof(header) + payload_length;
  u8 *packet = kmalloc(packet_length);
  memcpy(packet, header, sizeof(header));
  memcpy(packet + sizeof(header), payload, payload_length);
  send_ipv4_packet((ipv4_t){.d = dst->sin_addr.s_addr}, 0x11, packet,
                   packet_length);
  kfree(packet);
}

void handle_udp(ipv4_t src_ip, ipv4_t dst_ip, const u8 *payload,
                u32 packet_length) {
  (void)dst_ip;
  if (packet_length < sizeof(u16[4])) {
    return;
  }
  const u16 *header = (u16 *)payload;
  u16 src_port = ntohs(*header);
  u16 dst_port = ntohs(*(header + 1));
  u16 length = ntohs(*(header + 2));
  u16 checksum = ntohs(*(header + 3));
  (void)checksum; // TODO
  if (length < 8) {
    return;
  }
  if (length != packet_length) {
    return;
  }

  struct UdpConnection *con = udp_find_connection(src_ip, src_port, dst_port);
  if (!con) {
    return;
  }

  u32 len = length - 8 + sizeof(struct sockaddr_in);

  if (ringbuffer_unused(&con->incoming_buffer) < len + sizeof(u32)) {
    return;
  }

  ringbuffer_write(&con->incoming_buffer, (void *)&len, sizeof(len));

  struct sockaddr_in from;
  from.sin_addr.s_addr = src_ip.d;
  from.sin_port = src_port;
  ringbuffer_write(&con->incoming_buffer, (void *)&from, sizeof(from));
  ringbuffer_write(&con->incoming_buffer, payload + 8, length - 8);
}
