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
  send_ipv4_packet(dst->sin_addr.s_addr, 0x11, packet, packet_length);
  kfree(packet);
}

void handle_udp(u8 src_ip[4], const u8 *payload,
                u32 packet_length) {
  assert(packet_length >= 8);
  // n_.* means network format(big endian)
  // h_.* means host format((probably) little endian)
  u16 n_source_port = *(u16 *)payload;
  u16 h_source_port = ntohs(n_source_port);
  (void)h_source_port;
  u16 h_dst_port = ntohs(*(u16 *)(payload + 2));
  u16 h_length = ntohs(*(u16 *)(payload + 4));
  assert(h_length == packet_length);
  u16 data_length = h_length - 8;
  const u8 *data = payload + 8;

  // Find the open port
  OPEN_INET_SOCKET *in_s = find_open_udp_port(htons(h_dst_port));
  assert(in_s);
  SOCKET *s = in_s->s;
  vfs_fd_t *fifo_file = s->ptr_socket_fd;

  // Write the sockaddr struct such that it can later be
  // given to userland if asked.
  struct sockaddr_in /*{
    sa_family_t sin_family;
    union {
      u32 s_addr;
    } sin_addr;
    u16 sin_port;
  }*/ in;
  in.sin_family = AF_INET;
  memcpy(&in.sin_addr.s_addr, src_ip, sizeof(u32));
  in.sin_port = n_source_port;
  socklen_t sock_length = sizeof(struct sockaddr_in);

  raw_vfs_pwrite(fifo_file, &sock_length, sizeof(sock_length), 0);
  raw_vfs_pwrite(fifo_file, &in, sizeof(in), 0);

  // Write the UDP payload length(not including header)
  raw_vfs_pwrite(fifo_file, &data_length, sizeof(u16), 0);

  // Write the UDP payload
  raw_vfs_pwrite(fifo_file, (char *)data, data_length, 0);
}
