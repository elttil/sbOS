#include <assert.h>
#include <cpu/arch_inst.h>
#include <drivers/pit.h>
#include <fs/vfs.h>
#include <math.h>
#include <network/arp.h>
#include <network/bytes.h>
#include <network/ipv4.h>
#include <network/udp.h>
#include <random.h>

#define CWR (1 << 7)
#define ECE (1 << 6)
#define URG (1 << 5)
#define ACK (1 << 4)
#define PSH (1 << 3)
#define RST (1 << 2)
#define SYN (1 << 1)
#define FIN (1 << 0)

#define MSS 536

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

u16 tcp_calculate_checksum(ipv4_t src_ip, u32 dst_ip, const u8 *payload,
                           u16 payload_length, const struct TCP_HEADER *header,
                           int total) {
  if (total < header->data_offset + payload_length) {
    return 0;
  }

  int pseudo = sizeof(u32) * 2 + sizeof(u16) + sizeof(u8) * 2;

  int buffer_length =
      pseudo + header->data_offset * sizeof(u32) + payload_length;
  u8 buffer[buffer_length];
  u8 *ptr = buffer;
  memcpy(ptr, &src_ip.d, sizeof(u32));
  ptr += sizeof(u32);
  memcpy(ptr, &dst_ip, sizeof(u32));
  ptr += sizeof(u32);
  *ptr = 0;
  ptr += sizeof(u8);
  *ptr = 6;
  ptr += sizeof(u8);
  *(u16 *)ptr = htons(header->data_offset * sizeof(u32) + payload_length);
  ptr += sizeof(u16);
  memcpy(ptr, header, header->data_offset * sizeof(u32));
  memset(ptr + 16, 0, sizeof(u16)); // set checksum to zero
  ptr += header->data_offset * sizeof(u32);
  memcpy(ptr, payload, payload_length);

  return tcp_checksum((u16 *)buffer, buffer_length);
}

struct TcpPacket {
  u32 time;
  u32 seq_num;
  u16 payload_length;
  u8 *buffer;
  u16 length;
};

static void tcp_send(struct TcpConnection *con, u8 *buffer, u16 length,
                     u32 seq_num, u32 payload_length) {
  if (payload_length > 0) {
    struct TcpPacket *packet = kmalloc(sizeof(struct TcpPacket));
    assert(packet);
    packet->time = pit_num_ms();
    packet->seq_num = seq_num;
    packet->buffer = buffer;
    packet->length = length;
    packet->payload_length = payload_length;

    assert(relist_add(&con->inflight, packet, NULL));
  }
  send_ipv4_packet((ipv4_t){.d = con->outgoing_ip}, 6, buffer, length);
}

void tcp_resend_packets(struct TcpConnection *con) {
  if (con->inflight.num_entries > 0) {
    for (u32 i = 0;; i++) {
      struct TcpPacket *packet;
      int end;
      if (!relist_get(&con->inflight, i, (void *)&packet, &end)) {
        if (end) {
          break;
        }
        continue;
      }
      if (packet->time + 200 > pit_num_ms()) {
        continue;
      }
      // resend the packet
      relist_remove(&con->inflight, i);
      tcp_send(con, packet->buffer, packet->length, packet->seq_num,
               packet->payload_length);
      kfree(packet);
    }
  }
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
  header.checksum = tcp_calculate_checksum(
      ip_address, con->outgoing_ip, (const u8 *)payload, payload_length,
      &header, sizeof(struct TCP_HEADER) + payload_length);
  int send_len = sizeof(header) + payload_length;
  u8 *send_buffer = kmalloc(send_len);
  memcpy(send_buffer, &header, sizeof(header));
  memcpy(send_buffer + sizeof(header), payload, payload_length);

  tcp_send(con, send_buffer, send_len, con->seq, 0);
}

void tcp_close_connection(struct TcpConnection *con) {
  tcp_send_empty_payload(con, FIN | ACK);
}

void tcp_send_ack(struct TcpConnection *con) {
  tcp_send_empty_payload(con, ACK);
}

void tcp_send_syn(struct TcpConnection *con) {
  tcp_send_empty_payload(con, SYN);
  con->seq++;
}

int tcp_can_send(struct TcpConnection *con, u16 payload_length) {
  if (con->inflight.num_entries > 2) {
    tcp_resend_packets(con);
    return 0;
  }
  if (con->seq - con->seq_ack + payload_length > con->current_window_size) {
    tcp_resend_packets(con);
    return 0;
  }
  return 1;
}

int send_tcp_packet(struct TcpConnection *con, const u8 *payload,
                    u16 payload_length) {
  if (!tcp_can_send(con, payload_length)) {
    return 0;
  }

  if (payload_length > MSS) {
    if (0 == send_tcp_packet(con, payload, MSS)) {
      return 0;
    }
    payload_length -= MSS;
    payload += MSS;
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
  header.checksum = tcp_calculate_checksum(
      ip_address, con->outgoing_ip, (const u8 *)payload, payload_length,
      &header, sizeof(struct TCP_HEADER) + payload_length);
  int send_len = sizeof(header) + payload_length;
  u8 *send_buffer = kmalloc(send_len);
  assert(send_buffer);
  memcpy(send_buffer, &header, sizeof(header));
  memcpy(send_buffer + sizeof(header), payload, payload_length);

  tcp_send(con, send_buffer, send_len, con->seq, payload_length);

  con->seq += payload_length;
  return 1;
}

void handle_tcp(ipv4_t src_ip, ipv4_t dst_ip, const u8 *payload,
                u32 payload_length) {
  const struct TCP_HEADER *header = (const struct TCP_HEADER *)payload;
  u16 tcp_payload_length = payload_length - header->data_offset * sizeof(u32);
  const u8 *tcp_payload = payload + header->data_offset * sizeof(u32);
  u16 checksum =
      tcp_calculate_checksum(src_ip, dst_ip.d, tcp_payload, tcp_payload_length,
                             header, payload_length);
  if (header->checksum != checksum) {
    return;
  }
  u16 n_src_port = header->src_port;
  u16 n_dst_port = header->dst_port;
  u32 n_seq_num = header->seq_num;
  u32 n_ack_num = header->ack_num;
  u32 n_window_size = header->window_size;

  u8 flags = header->flags;

  u16 src_port = htons(n_src_port);
  u16 dst_port = htons(n_dst_port);
  u32 seq_num = htonl(n_seq_num);
  u32 ack_num = htonl(n_ack_num);
  u16 window_size = htons(n_window_size);

  (void)ack_num;

  if (SYN == flags) {
    struct TcpConnection *con =
        internal_tcp_incoming(src_ip.d, src_port, 0, dst_port);
    if (!con) {
      return;
    }
    con->window_size = window_size;
    con->current_window_size = MSS;
    con->ack = seq_num + 1;
    tcp_send_empty_payload(con, SYN | ACK);
    con->seq++;
    return;
  }

  struct TcpConnection *incoming_connection =
      tcp_find_connection(src_ip, src_port, dst_port);
  if (!incoming_connection) {
    kprintf("unable to find open port for incoming connection\n");
    return;
  }
  incoming_connection->window_size = window_size;
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
    return;
  }
  if (ACK & flags) {
    incoming_connection->seq_ack = ack_num;
    if (incoming_connection->inflight.num_entries > 0) {
      for (u32 i = 0;; i++) {
        struct TcpPacket *packet;
        int end;
        if (!relist_get(&incoming_connection->inflight, i, (void *)&packet,
                        &end)) {
          if (end) {
            break;
          }
          continue;
        }
        if (packet->seq_num + packet->payload_length != ack_num) {
          continue;
        }
        relist_remove(&incoming_connection->inflight, i);
        kfree(packet->buffer);
        kfree(packet);
        break;
      }
    }
    tcp_resend_packets(incoming_connection);
    if (0 == incoming_connection->inflight.num_entries) {
      u32 rest = incoming_connection->window_size -
                 incoming_connection->current_window_size;
      if (rest > 0) {
        incoming_connection->current_window_size += min(rest, MSS);
      }
    }
  }
  if (tcp_payload_length > 0) {
    u32 len = ringbuffer_write(&incoming_connection->incoming_buffer,
                               tcp_payload, tcp_payload_length);
    assert(len == tcp_payload_length);
    incoming_connection->ack += len;
    tcp_send_ack(incoming_connection);
  }
  if (0 != (flags & FIN)) {
    incoming_connection->ack++;

    tcp_send_empty_payload(incoming_connection, FIN | ACK);
    incoming_connection->seq++;

    incoming_connection->dead = 1; // FIXME: It should wait for a ACK
                                   // of the FIN before the connection
                                   // is closed.
  }
}
