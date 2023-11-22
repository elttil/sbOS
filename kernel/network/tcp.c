#include <assert.h>
#include <drivers/pit.h>
#include <network/bytes.h>
#include <network/ipv4.h>
#include <network/udp.h>
#include <socket.h>
extern u8 ip_address[4];

#define CWR (1 << 7)
#define ECE (1 << 6)
#define URG (1 << 5)
#define ACK (1 << 4)
#define PSH (1 << 3)
#define RST (1 << 2)
#define SYN (1 << 1)
#define FIN (1 << 0)

// FIXME: This should be dynamic
#define WINDOW_SIZE 4096

struct __attribute__((__packed__)) TCP_HEADER {
  u16 src_port;
  u16 dst_port;
  u32 seq_num;
  u32 ack_num;
  u8 reserved : 4;
  u8 data_offset : 4;
  u8 flags;
  u16 window_size;
  u16 checksum;
  u16 urgent_pointer;
};

struct __attribute__((__packed__)) PSEUDO_TCP_HEADER {
  u32 src_addr;
  u32 dst_addr;
  u8 zero;
  u8 protocol;
  u16 tcp_length;
  u16 src_port;
  u16 dst_port;
  u32 seq_num;
  u32 ack_num;
  u8 reserved : 4;
  u8 data_offset : 4;
  u8 flags;
  u16 window_size;
  u16 checksum_zero; // ?????
  u16 urgent_pointer;
};

u16 tcp_checksum(u16 *buffer, int size) {
  unsigned long cksum = 0;
  while (size > 1) {
    cksum += *buffer++;
    size -= sizeof(u16);
  }
  if (size)
    cksum += *(u8 *)buffer;

  cksum = (cksum >> 16) + (cksum & 0xffff);
  cksum += (cksum >> 16);
  return (u16)(~cksum);
}

void tcp_calculate_checksum(u8 src_ip[4], u8 dst_ip[4], const u8 *payload,
                            u16 payload_length, struct TCP_HEADER *header) {
  struct PSEUDO_TCP_HEADER ps = {0};
  memcpy(&ps.src_addr, src_ip, sizeof(u32));
  memcpy(&ps.dst_addr, dst_ip, sizeof(u32));
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
  u8 buffer[buffer_length];
  memcpy(buffer, &ps, sizeof(ps));
  memcpy(buffer + sizeof(ps), payload, payload_length);
  header->checksum = tcp_checksum((u16 *)buffer, buffer_length);
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
  header.window_size = htons(WINDOW_SIZE);
  header.urgent_pointer = 0;
  u32 dst_ip;
  memcpy(&dst_ip, inc->ip, sizeof(dst_ip));
  u8 payload[0];
  u16 payload_length = 0;
  tcp_calculate_checksum(ip_address, inc->ip, (const u8 *)payload,
                         payload_length, &header);
  int send_len = sizeof(header) + payload_length;
  u8 send_buffer[send_len];
  memcpy(send_buffer, &header, sizeof(header));
  memcpy(send_buffer + sizeof(header), payload, payload_length);
  send_ipv4_packet(dst_ip, 6, send_buffer, send_len);
}

void send_tcp_packet(struct INCOMING_TCP_CONNECTION *inc, u8 *payload,
                     u16 payload_length) {
  struct TCP_HEADER header = {0};
  header.src_port = htons(inc->dst_port);
  header.dst_port = inc->n_port;
  header.seq_num = htonl(inc->seq_num);
  header.ack_num = htonl(inc->ack_num);
  header.data_offset = 5;
  header.reserved = 0;
  header.flags = PSH | ACK;
  header.window_size = htons(WINDOW_SIZE);
  header.urgent_pointer = 0;
  u32 dst_ip;
  memcpy(&dst_ip, inc->ip, sizeof(dst_ip));
  tcp_calculate_checksum(ip_address, inc->ip, (const u8 *)payload,
                         payload_length, &header);
  int send_len = sizeof(header) + payload_length;
  u8 send_buffer[send_len];
  memcpy(send_buffer, &header, sizeof(header));
  memcpy(send_buffer + sizeof(header), payload, payload_length);
  send_ipv4_packet(dst_ip, 6, send_buffer, send_len);

  inc->seq_num += payload_length;
}

void send_empty_tcp_message(struct INCOMING_TCP_CONNECTION *inc, u8 flags,
                            u32 inc_seq_num, u16 n_dst_port, u16 n_src_port) {
  struct TCP_HEADER header = {0};
  header.src_port = n_dst_port;
  header.dst_port = n_src_port;
  header.seq_num = 0;
  header.ack_num = htonl(inc_seq_num + 1);
  header.data_offset = 5;
  header.reserved = 0;
  header.flags = flags;
  header.window_size = htons(WINDOW_SIZE);
  header.urgent_pointer = 0;
  char payload[0];
  tcp_calculate_checksum(ip_address, inc->ip, (const u8 *)payload, 0, &header);
  u32 dst_ip;
  memcpy(&dst_ip, inc->ip, sizeof(dst_ip));
  send_ipv4_packet(dst_ip, 6, (const u8 *)&header, sizeof(header));
}

void handle_tcp(u8 src_ip[4], const u8 *payload, u32 payload_length) {
  const struct TCP_HEADER *inc_header = (const struct TCP_HEADER *)payload;
  u16 n_src_port = *(u16 *)(payload);
  u16 n_dst_port = *(u16 *)(payload + 2);
  u32 n_seq_num = *(u32 *)(payload + 4);
  u32 n_ack_num = *(u32 *)(payload + 8);

  u8 flags = *(payload + 13);

  u16 src_port = htons(n_src_port);
  (void)src_port;
  u16 dst_port = htons(n_dst_port);
  u32 seq_num = htonl(n_seq_num);
  u32 ack_num = htonl(n_ack_num);
  (void)ack_num;

  if (SYN == flags) {
    kprintf("GOT SYN UPTIME: %d\n", pit_num_ms());
    struct INCOMING_TCP_CONNECTION *inc;
    if (!(inc = handle_incoming_tcp_connection(src_ip, n_src_port, dst_port)))
      return;
    memcpy(inc->ip, src_ip, sizeof(u8[4]));
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
    u16 tcp_payload_length =
        payload_length - inc_header->data_offset * sizeof(u32);
    fifo_object_write((u8 *)(payload + inc_header->data_offset * sizeof(u32)),
                      0, tcp_payload_length, inc->data_file);
    *inc->has_data_ptr = 1;

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
    header.window_size = htons(WINDOW_SIZE);
    header.urgent_pointer = 0;
    char payload[0];
    tcp_calculate_checksum(ip_address, src_ip, (const u8 *)payload, 0, &header);
    u32 dst_ip;
    memcpy(&dst_ip, src_ip, sizeof(dst_ip));
    send_ipv4_packet(dst_ip, 6, (const u8 *)&header, sizeof(header));
    return;
  }
}
