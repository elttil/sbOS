#include <assert.h>
#include <drivers/pit.h>
#include <network/bytes.h>
#include <network/ipv4.h>
#include <network/udp.h>
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

void tcp_wait_reply(struct TcpConnection *con) {
  for (;;) {
    if (con->unhandled_packet) {
      return;
    }
    // TODO: Make the scheduler halt the process
    switch_task();
  }
}

u16 tcp_checksum(u16 *buffer, int size) {
  unsigned long cksum = 0;
  while (size > 1) {
    cksum += *buffer++;
    size -= sizeof(u16);
  }
  if (size) {
    cksum += *(u8 *)buffer;
  }

  cksum = (cksum >> 16) + (cksum & 0xffff);
  cksum += (cksum >> 16);
  return (u16)(~cksum);
}

void tcp_calculate_checksum(u8 src_ip[4], u32 dst_ip, const u8 *payload,
                            u16 payload_length, struct TCP_HEADER *header) {
  struct PSEUDO_TCP_HEADER ps = {0};
  memcpy(&ps.src_addr, src_ip, sizeof(u32));
  memcpy(&ps.dst_addr, &dst_ip, sizeof(u32));
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

void tcp_send_empty_payload(struct TcpConnection *con, u8 flags) {
  struct TCP_HEADER header = {0};
  header.src_port = htons(con->incoming_port);
  header.dst_port = htons(con->outgoing_port);
  header.seq_num = htonl(con->seq);
  header.ack_num = htonl(con->ack);
  header.data_offset = 5;
  header.reserved = 0;
  header.flags = flags;
  header.window_size = htons(WINDOW_SIZE);
  header.urgent_pointer = 0;

  u8 payload[0];
  u16 payload_length = 0;
  tcp_calculate_checksum(ip_address, con->outgoing_ip, (const u8 *)payload,
                         payload_length, &header);
  int send_len = sizeof(header) + payload_length;
  u8 send_buffer[send_len];
  memcpy(send_buffer, &header, sizeof(header));
  memcpy(send_buffer + sizeof(header), payload, payload_length);

  send_ipv4_packet(con->outgoing_ip, 6, send_buffer, send_len);
}

void tcp_send_ack(struct TcpConnection *con) {
  tcp_send_empty_payload(con, ACK);
}

void tcp_send_syn(struct TcpConnection *con) {
  tcp_send_empty_payload(con, SYN);
  con->seq++;
}

// void send_tcp_packet(struct INCOMING_TCP_CONNECTION *inc, const u8 *payload,
//                      u16 payload_length) {
void send_tcp_packet(struct TcpConnection *con, const u8 *payload,
                     u16 payload_length) {
  if (payload_length > 1500 - 20 - sizeof(struct TCP_HEADER)) {
    send_tcp_packet(con, payload, 1500 - 20 - sizeof(struct TCP_HEADER));
    payload_length -= 1500 - 20 - sizeof(struct TCP_HEADER);
    payload += 1500 - 20 - sizeof(struct TCP_HEADER);
    return send_tcp_packet(con, payload, payload_length);
  }
  struct TCP_HEADER header = {0};
  header.src_port = htons(con->incoming_port);
  header.dst_port = htons(con->outgoing_port);
  header.seq_num = htonl(con->seq);
  header.ack_num = htonl(con->ack);
  header.data_offset = 5;
  header.reserved = 0;
  header.flags = PSH | ACK;
  header.window_size = htons(WINDOW_SIZE);
  header.urgent_pointer = 0;
  tcp_calculate_checksum(ip_address, con->outgoing_ip, (const u8 *)payload,
                         payload_length, &header);
  int send_len = sizeof(header) + payload_length;
  u8 send_buffer[send_len];
  memcpy(send_buffer, &header, sizeof(header));
  memcpy(send_buffer + sizeof(header), payload, payload_length);
  send_ipv4_packet(con->outgoing_ip, 6, send_buffer, send_len);

  con->seq += payload_length;
}

void handle_tcp(u8 src_ip[4], const u8 *payload, u32 payload_length) {
  const struct TCP_HEADER *header = (const struct TCP_HEADER *)payload;
  (void)header;
  u16 n_src_port = *(u16 *)(payload);
  u16 n_dst_port = *(u16 *)(payload + 2);
  u32 n_seq_num = *(u32 *)(payload + 4);
  u32 n_ack_num = *(u32 *)(payload + 8);

  //  u8 flags = *(payload + 13);
  u8 flags = header->flags;

  u16 src_port = htons(n_src_port);
  (void)src_port;
  u16 dst_port = htons(n_dst_port);
  u32 seq_num = htonl(n_seq_num);
  u32 ack_num = htonl(n_ack_num);
  (void)ack_num;

  if (SYN == flags) {
    u32 t;
    memcpy(&t, src_ip, sizeof(u8[4]));
    struct TcpConnection *con = internal_tcp_incoming(t, src_port, 0, dst_port);
    assert(con);
    con->ack = seq_num + 1;
    tcp_send_empty_payload(con, SYN | ACK);
    return;
  }

  struct TcpConnection *incoming_connection = tcp_find_connection(dst_port);
  kprintf("dst_port: %d\n", dst_port);
  if (incoming_connection) {
    incoming_connection->unhandled_packet = 1;
    if (0 != (flags & RST)) {
      klog("Requested port is closed", LOG_NOTE);
      incoming_connection->dead = 1;
      return;
    }
    if (ACK == flags) {
      if (0 == incoming_connection->handshake_state) {
        // Then it is probably a response to the SYN|ACK we sent.
        incoming_connection->handshake_state = 1;
        return;
      }
    }
    if ((SYN | ACK) == flags) {
      assert(0 == incoming_connection->handshake_state);
      incoming_connection->handshake_state = 1;

      incoming_connection->ack = seq_num + 1;

      tcp_send_ack(incoming_connection);
    }
    //    if (0 != (flags & PSH)) {
    u16 tcp_payload_length = payload_length - header->data_offset * sizeof(u32);
    if (tcp_payload_length > 0) {
      int len = fifo_object_write(
          (u8 *)(payload + header->data_offset * sizeof(u32)), 0,
          tcp_payload_length, incoming_connection->data_file);
      assert(len >= 0);
      incoming_connection->ack += len;
      tcp_send_ack(incoming_connection);
    }
    if (0 != (flags & FIN)) {
      incoming_connection->ack++;

      tcp_send_empty_payload(incoming_connection, FIN | ACK);

      incoming_connection->dead = 1; // FIXME: It should wait for a ACK
                                     // of the FIN before the connection
                                     // is closed.
    }
  } else {
    return;
  }
}
/*
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
    struct INCOMING_TCP_CONNECTION *inc;
    if (!(inc = handle_incoming_tcp_connection(src_ip, n_src_port, dst_port))) {
      return;
    }
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
  if (!inc) {
    return;
  }
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
    kprintf("TCP: Got PSH\n");
    u16 tcp_payload_length =
        payload_length - inc_header->data_offset * sizeof(u32);
    int rc = fifo_object_write(
        (u8 *)(payload + inc_header->data_offset * sizeof(u32)), 0,
        tcp_payload_length, inc->data_file);
    kprintf("fifo object write rc: %x\n", rc);
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
    u32 dst_ip;
    memcpy(&dst_ip, src_ip, sizeof(dst_ip));
    char payload[0];
    tcp_calculate_checksum(ip_address, dst_ip, (const u8 *)payload, 0, &header);
    send_ipv4_packet(dst_ip, 6, (const u8 *)&header, sizeof(header));
    return;
  }
}*/
