#include <assert.h>
#include <cpu/arch_inst.h>
#include <drivers/pit.h>
#include <fs/vfs.h>
#include <interrupts.h>
#include <math.h>
#include <network/arp.h>
#include <network/bytes.h>
#include <network/ipv4.h>
#include <network/tcp.h>
#include <network/udp.h>
#include <random.h>

#define MSS 536

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

u16 tcp_checksum(u16 *buffer, u32 size) {
  u32 cksum = 0;
  for (; size > 1;) {
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

static void tcp_send(struct TcpConnection *con, u8 *buffer, u16 length,
                     u32 seq_num, u32 payload_length) {
  send_ipv4_packet((ipv4_t){.d = con->outgoing_ip}, 6, buffer, length);
}

void tcp_send_empty_payload(struct TcpConnection *con, u8 flags) {
  struct TCP_HEADER header = {0};
  header.src_port = htons(con->incoming_port);
  header.dst_port = htons(con->outgoing_port);
  header.seq_num = htonl(con->snd_nxt);
  if (flags & ACK) {
    con->should_send_ack = 0;
    header.ack_num = htonl(con->rcv_nxt);
  } else {
    header.ack_num = 0;
  }
  header.data_offset = 5;
  header.reserved = 0;
  header.flags = flags;
  header.window_size = htons(con->rcv_wnd);
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

  tcp_send(con, send_buffer, send_len, con->snd_nxt, 0);

  con->snd_nxt += (flags & SYN) ? 1 : 0;
  con->snd_nxt += (flags & FIN) ? 1 : 0;
  con->snd_max = max(con->snd_nxt, con->snd_max);
}

// When both the client and the server have closed the connection it can
// be "destroyed".
void tcp_destroy_connection(struct TcpConnection *con) {
  con->state = TCP_STATE_CLOSED;
  tcp_remove_connection(con);
}

void tcp_close_connection(struct TcpConnection *con) {
  if (TCP_STATE_CLOSE_WAIT == con->state) {
    tcp_send_empty_payload(con, FIN);
    con->state = TCP_STATE_LAST_ACK;
    return;
  }
  if (TCP_STATE_ESTABLISHED == con->state) {
    tcp_send_empty_payload(con, FIN);
    con->state = TCP_STATE_FIN_WAIT1;
    return;
  }
  if (TCP_STATE_SYN_RECIEVED == con->state) {
    tcp_send_empty_payload(con, FIN);
    con->state = TCP_STATE_FIN_WAIT1;
    return;
  }
  if (TCP_STATE_SYN_SENT == con->state) {
    tcp_destroy_connection(con);
    return;
  }
}

u16 tcp_can_send(struct TcpConnection *con) {
  if (TCP_STATE_CLOSED == con->state) {
    return 0;
  }
  return con->snd_una + con->snd_wnd - con->snd_max;
}

int send_tcp_packet(struct TcpConnection *con, const u8 *payload,
                    u16 payload_length) {
  if (0 == tcp_can_send(con)) {
    return 0;
  }

  u16 len = min(1000, payload_length);
  if (payload_length > len) {
    if (0 == send_tcp_packet(con, payload, len)) {
      return 0;
    }
    payload_length -= len;
    payload += len;
    return send_tcp_packet(con, payload, payload_length);
  }
  struct TCP_HEADER header = {0};
  header.src_port = htons(con->incoming_port);
  header.dst_port = htons(con->outgoing_port);
  header.seq_num = htonl(con->snd_nxt);
  header.ack_num = htonl(con->rcv_nxt);
  header.data_offset = 5;
  header.reserved = 0;
  header.flags = PSH | ACK;
  header.window_size = htons(con->rcv_wnd);
  header.urgent_pointer = 0;
  header.checksum = tcp_calculate_checksum(
      ip_address, con->outgoing_ip, (const u8 *)payload, payload_length,
      &header, sizeof(struct TCP_HEADER) + payload_length);
  int send_len = sizeof(header) + payload_length;
  u8 *send_buffer = kmalloc(send_len);
  assert(send_buffer);
  memcpy(send_buffer, &header, sizeof(header));
  memcpy(send_buffer + sizeof(header), payload, payload_length);

  tcp_send(con, send_buffer, send_len, con->snd_nxt, payload_length);

  con->snd_nxt += payload_length;
  con->snd_max = max(con->snd_nxt, con->snd_max);
  return 1;
}

void handle_tcp(ipv4_t src_ip, ipv4_t dst_ip, const u8 *payload,
                u32 payload_length) {
  const struct TCP_HEADER *header = (const struct TCP_HEADER *)payload;
  u32 tcp_payload_length = payload_length - header->data_offset * sizeof(u32);
  const u8 *tcp_payload = payload + header->data_offset * sizeof(u32);
  u16 checksum =
      tcp_calculate_checksum(src_ip, dst_ip.d, tcp_payload, tcp_payload_length,
                             header, payload_length);
  if (header->checksum != checksum) {
    klog(LOG_WARN, "Bad TCP checksum");
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
  struct TcpConnection *con =
      tcp_find_connection(src_ip, src_port, dst_ip, dst_port);
  if (!con) {
    return;
  }

  if (con->state == TCP_STATE_LISTEN || con->state == TCP_STATE_SYN_SENT) {
    con->rcv_nxt = seq_num;
  }

  if (ack_num > con->snd_max) {
    // TODO: Odd ACK number, what should be done?
    kprintf("odd ACK\n");
    return;
  }

  if (ack_num < con->snd_una) {
    // TODO duplicate ACK
    kprintf("duplicate ACK\n");
    return;
  }

  if (con->rcv_nxt != seq_num) {
    return;
  }

  con->snd_wnd = window_size;
  con->snd_una = ack_num;

  con->rcv_nxt += (FIN & flags) ? 1 : 0;
  con->rcv_nxt += (SYN & flags) ? 1 : 0;

  switch (con->state) {
  case TCP_STATE_LISTEN: {
    if (SYN & flags) {
      tcp_send_empty_payload(con, SYN | ACK);
      con->state = TCP_STATE_SYN_RECIEVED;
      break;
    }
    break;
  }
  case TCP_STATE_SYN_RECIEVED: {
    if (ACK & flags) {
      con->state = TCP_STATE_ESTABLISHED;
      break;
    }
    break;
  }
  case TCP_STATE_SYN_SENT: {
    if ((ACK & flags) && (SYN & flags)) {
      tcp_send_empty_payload(con, ACK);
      con->state = TCP_STATE_ESTABLISHED;
      break;
    }
    break;
  }
  case TCP_STATE_ESTABLISHED: {
    if (FIN & flags) {
      tcp_send_empty_payload(con, ACK);
      con->state = TCP_STATE_CLOSE_WAIT;
      break;
    }
    if (tcp_payload_length > 0) {
      if (tcp_payload_length > ringbuffer_unused(&con->incoming_buffer)) {
        return;
      }
      int rc = ringbuffer_write(&con->incoming_buffer, tcp_payload,
                                tcp_payload_length);
      con->rcv_nxt += rc;
      con->should_send_ack = 1;
    }
    break;
  }
  case TCP_STATE_FIN_WAIT1: {
    if ((ACK & flags) && (FIN & flags)) {
      tcp_send_empty_payload(con, ACK);
      con->state = TCP_STATE_TIME_WAIT;
      break;
    }
    if (ACK & flags) {
      con->state = TCP_STATE_FIN_WAIT2;
      break;
    }
    if (FIN & flags) {
      tcp_send_empty_payload(con, ACK);
      con->state = TCP_STATE_CLOSING;
      break;
    }
    break;
  }
  case TCP_STATE_LAST_ACK: {
    if (ACK & flags) {
      tcp_destroy_connection(con);
      break;
    }
    break;
  }
  case TCP_STATE_CLOSE_WAIT: {
    // Waiting for this machine to close the connection. There is
    // nothing to respond with.
    break;
  }
  default: {
    klog(LOG_WARN, "TCP state not handled %d", con->state);
    break;
  }
  };
}
