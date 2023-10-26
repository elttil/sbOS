#include <assert.h>
#include <network/bytes.h>
#include <network/udp.h>
#include <socket.h>

void handle_udp(const uint8_t *payload, uint32_t packet_length) {
  assert(packet_length >= 8);
  uint16_t source_port = ntohs(*(uint16_t *)payload);
  uint16_t dst_port = ntohs(*(uint16_t *)(payload + 2));
  uint16_t length = ntohs(*(uint16_t *)(payload + 4));
  assert(length == packet_length);
  kprintf("source_port: %d\n", source_port);
  kprintf("dst_port: %d\n", dst_port);
  uint32_t data_length = length - 8;
  const uint8_t *data = payload + 8;

  // Find the open port
  OPEN_INET_SOCKET *in_s = find_open_udp_port(htons(dst_port));
  assert(in_s);
  SOCKET *s = in_s->s;
  vfs_fd_t *fifo_file = s->ptr_socket_fd;
  raw_vfs_pwrite(fifo_file, (char *)data, data_length, 0);
  for (uint32_t i = 0; i < data_length; i++) {
    kprintf("%c", data[i]);
  }
}
