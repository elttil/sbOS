#include <assert.h>
#include <drivers/pit.h>
#include <network/bytes.h>
#include <network/ipv4.h>
#include <network/udp.h>
#include <socket.h>
extern uint8_t ip_address[4];

#define CWR (1 << 7)
#define ECE (1 << 6)
#define URG (1 << 5)
#define ACK (1 << 4)
#define PSH (1 << 3)
#define RST (1 << 2)
#define SYN (1 << 1)
#define FIN (1 << 0)

struct __attribute__((__packed__)) TCP_HEADER {
  uint16_t src_port;
  uint16_t dst_port;
  uint32_t seq_num;
  uint32_t ack_num;
  uint8_t reserved : 4;
  uint8_t data_offset : 4;
  uint8_t flags;
  uint16_t window_size;
  uint16_t checksum;
  uint16_t urgent_pointer;
};

struct __attribute__((__packed__)) PSEUDO_TCP_HEADER {
  uint32_t src_addr;
  uint32_t dst_addr;
  uint8_t zero;
  uint8_t protocol;
  uint16_t tcp_length;
  uint16_t src_port;
  uint16_t dst_port;
  uint32_t seq_num;
  uint32_t ack_num;
  uint8_t reserved : 4;
  uint8_t data_offset : 4;
  uint8_t flags;
  uint16_t window_size;
  uint16_t checksum_zero; // ?????
  uint16_t urgent_pointer;
};

uint16_t tcp_checksum(uint16_t *buffer, int size) {
  unsigned long cksum = 0;
  while (size > 1) {
    cksum += *buffer++;
    size -= sizeof(uint16_t);
  }
  if (size)
    cksum += *(uint8_t *)buffer;

  cksum = (cksum >> 16) + (cksum & 0xffff);
  cksum += (cksum >> 16);
  return (uint16_t)(~cksum);
}

void tcp_calculate_checksum(uint8_t src_ip[4], uint8_t dst_ip[4],
                            const uint8_t *payload, uint16_t payload_length,
                            struct TCP_HEADER *header) {
  struct PSEUDO_TCP_HEADER ps = {0};
  memcpy(&ps.src_addr, src_ip, sizeof(uint32_t));
  memcpy(&ps.dst_addr, dst_ip, sizeof(uint32_t));
  ps.protocol = 6;
  ps.tcp_length = htons(20 + payload_length);
  ps.src_port = header->src_port;
  ps.dst_port = header->dst_port;
  ps.seq_num = header->seq_num;
  ps.ack_num = header->ack_num;
  ps.data_offset = header->data_offset;
  ps.reserved = header->reserved;
  ps.flags = header->flags;
  ps.window_size = header->window_size;
  ps.urgent_pointer = header->urgent_pointer;
  //  ps.options = 0;
  int buffer_length = sizeof(ps) + payload_length;
  uint8_t buffer[buffer_length];
  memcpy(buffer, &ps, sizeof(ps));
  memcpy(buffer + sizeof(ps), payload, payload_length);
  header->checksum = tcp_checksum((uint16_t *)buffer, buffer_length);
}

void tcp_close_connection(struct INCOMING_TCP_CONNECTION *inc) {
  struct TCP_HEADER header = {0};
  header.src_port = htons(inc->dst_port);
  header.dst_port = inc->n_port;
  header.seq_num = htonl(inc->seq_num);
  header.ack_num = htonl(inc->ack_num);
  header.data_offset = 5;
  header.reserved = 0;
  header.flags = FIN | ACK;
  header.window_size = htons(512);
  header.urgent_pointer = 0;
  uint32_t dst_ip;
  memcpy(&dst_ip, inc->ip, sizeof(dst_ip));
  uint8_t payload[0];
  uint16_t payload_length = 0;
  tcp_calculate_checksum(ip_address, inc->ip, (const uint8_t *)payload,
                         payload_length, &header);
  int send_len = sizeof(header) + payload_length;
  uint8_t send_buffer[send_len];
  memcpy(send_buffer, &header, sizeof(header));
  memcpy(send_buffer + sizeof(header), payload, payload_length);
  send_ipv4_packet(dst_ip, 6, send_buffer, send_len);
}

void send_tcp_packet(struct INCOMING_TCP_CONNECTION *inc, uint8_t *payload,
                     uint16_t payload_length) {
  struct TCP_HEADER header = {0};
  header.src_port = htons(inc->dst_port);
  header.dst_port = inc->n_port;
  header.seq_num = htonl(inc->seq_num);
  header.ack_num = htonl(inc->ack_num);
  header.data_offset = 5;
  header.reserved = 0;
  header.flags = PSH | ACK;
  header.window_size = htons(512);
  header.urgent_pointer = 0;
  uint32_t dst_ip;
  memcpy(&dst_ip, inc->ip, sizeof(dst_ip));
  tcp_calculate_checksum(ip_address, inc->ip, (const uint8_t *)payload,
                         payload_length, &header);
  int send_len = sizeof(header) + payload_length;
  uint8_t send_buffer[send_len];
  memcpy(send_buffer, &header, sizeof(header));
  memcpy(send_buffer + sizeof(header), payload, payload_length);
  send_ipv4_packet(dst_ip, 6, send_buffer, send_len);

  inc->seq_num += payload_length;
}

void send_empty_tcp_message(struct INCOMING_TCP_CONNECTION *inc, uint8_t flags,
                            uint32_t inc_seq_num, uint16_t n_dst_port,
                            uint16_t n_src_port) {
  struct TCP_HEADER header = {0};
  header.src_port = n_dst_port;
  header.dst_port = n_src_port;
  header.seq_num = 0;
  header.ack_num = htonl(inc_seq_num + 1);
  header.data_offset = 5;
  header.reserved = 0;
  header.flags = flags;
  header.window_size = htons(512); // TODO: What does this actually mean?
  header.urgent_pointer = 0;
  char payload[0];
  tcp_calculate_checksum(ip_address, inc->ip, (const uint8_t *)payload, 0,
                         &header);
  uint32_t dst_ip;
  memcpy(&dst_ip, inc->ip, sizeof(dst_ip));
  send_ipv4_packet(dst_ip, 6, (const uint8_t *)&header, sizeof(header));
}

void handle_tcp(uint8_t src_ip[4], const uint8_t *payload,
                uint32_t payload_length) {
  const struct TCP_HEADER *inc_header = (const struct TCP_HEADER *)payload;
  uint16_t n_src_port = *(uint16_t *)(payload);
  uint16_t n_dst_port = *(uint16_t *)(payload + 2);
  uint32_t n_seq_num = *(uint32_t *)(payload + 4);
  uint32_t n_ack_num = *(uint32_t *)(payload + 8);

  uint8_t flags = *(payload + 13);

  uint16_t src_port = htons(n_src_port);
  (void)src_port;
  uint16_t dst_port = htons(n_dst_port);
  uint32_t seq_num = htonl(n_seq_num);
  uint32_t ack_num = htonl(n_ack_num);
  (void)ack_num;

  if (SYN == flags) {
    kprintf("GOT SYN UPTIME: %d\n", pit_num_ms());
    struct INCOMING_TCP_CONNECTION *inc;
    if (!(inc = handle_incoming_tcp_connection(src_ip, n_src_port, dst_port)))
      return;
    memcpy(inc->ip, src_ip, sizeof(uint8_t[4]));
    inc->seq_num = 0;
    inc->ack_num = seq_num + 1;
    inc->connection_closed = 0;
    inc->requesting_connection_close = 0;
    send_empty_tcp_message(inc, SYN | ACK, seq_num, n_dst_port, n_src_port);
    inc->seq_num++;
    return;
  }
  struct INCOMING_TCP_CONNECTION *inc =
      get_incoming_tcp_connection(src_ip, n_src_port);
  if (!inc)
    return;
  if (flags == (FIN | ACK)) {
    if (inc->requesting_connection_close) {
      send_empty_tcp_message(inc, ACK, seq_num, n_dst_port, n_src_port);
      inc->connection_closed = 1;
    } else {
      send_empty_tcp_message(inc, FIN | ACK, seq_num, n_dst_port, n_src_port);
    }
    inc->seq_num++;
    inc->connection_closed = 1;
    return;
  }
  if (flags & ACK) {
    // inc->seq_num = ack_num;
  }
  if (flags & PSH) {
    kprintf("send ipv4 packet: %x\n", pit_num_ms());
    uint16_t tcp_payload_length =
        payload_length - inc_header->data_offset * sizeof(uint32_t);
    fifo_object_write(
        (uint8_t *)(payload + inc_header->data_offset * sizeof(uint32_t)), 0,
        tcp_payload_length, inc->data_file);

    // Send back a ACK
    struct TCP_HEADER header = {0};
    header.src_port = n_dst_port;
    header.dst_port = n_src_port;
    header.seq_num = htonl(inc->seq_num);
    inc->ack_num = seq_num + tcp_payload_length;
    header.ack_num = htonl(seq_num + tcp_payload_length);
    header.data_offset = 5;
    header.reserved = 0;
    header.flags = ACK;
    header.window_size = htons(512); // TODO: What does this actually mean?
    header.urgent_pointer = 0;
    char payload[0];
    tcp_calculate_checksum(ip_address, src_ip, (const uint8_t *)payload, 0,
                           &header);
    uint32_t dst_ip;
    memcpy(&dst_ip, src_ip, sizeof(dst_ip));
    send_ipv4_packet(dst_ip, 6, (const uint8_t *)&header, sizeof(header));
    kprintf("after ipv4 packet: %x\n", pit_num_ms());
    return;
  }
}
