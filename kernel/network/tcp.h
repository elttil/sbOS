#include <socket.h>
void tcp_send_syn(struct TcpConnection *con);
void tcp_wait_reply(struct TcpConnection *con);
void handle_tcp(u8 src_ip[4], const u8 *payload, u32 payload_length);
void send_tcp_packet(struct TcpConnection *con, const u8 *payload,
                     u16 payload_length);
  void tcp_close_connection(struct INCOMING_TCP_CONNECTION * s);
