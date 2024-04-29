#include <socket.h>
#include <typedefs.h>
void tcp_send_syn(struct TcpConnection *con);
void tcp_wait_reply(struct TcpConnection *con);
void handle_tcp(ipv4_t src_ip, const u8 *payload, u32 payload_length);
void send_tcp_packet(struct TcpConnection *con, const u8 *payload,
                     u16 payload_length);
void tcp_close_connection(struct TcpConnection *con);
