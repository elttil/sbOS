#include <assert.h>
#include <network/bytes.h>
#include <network/udp.h>
#include <socket.h>

void handle_udp(uint8_t src_ip[4], const uint8_t *payload,
                uint32_t packet_length) {
  assert(packet_length >= 8);
  // n_.* means network format(big endian)
  // h_.* means host format((probably) little endian)
  uint16_t n_source_port = *(uint16_t *)payload;
  uint16_t h_source_port = ntohs(n_source_port);
  uint16_t h_dst_port = ntohs(*(uint16_t *)(payload + 2));
  uint16_t h_length = ntohs(*(uint16_t *)(payload + 4));
  assert(h_length == packet_length);
  kprintf("source_port: %d\n", h_source_port);
  kprintf("dst_port: %d\n", h_dst_port);
  uint16_t data_length = h_length - 8;
  const uint8_t *data = payload + 8;

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
      uint32_t s_addr;
    } sin_addr;
    uint16_t sin_port;
  }*/ in;
  in.sin_family = AF_INET;
  memcpy(&in.sin_addr.s_addr, src_ip, sizeof(uint32_t));
  in.sin_port = n_source_port;
  socklen_t sock_length = sizeof(struct sockaddr_in);

  raw_vfs_pwrite(fifo_file, &sock_length, sizeof(sock_length), 0);
  raw_vfs_pwrite(fifo_file, &in, sizeof(in), 0);

  // Write the UDP payload length(not including header)
  raw_vfs_pwrite(fifo_file, &data_length, sizeof(uint16_t), 0);

  // Write the UDP payload
  raw_vfs_pwrite(fifo_file, (char *)data, data_length, 0);
}
