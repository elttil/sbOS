#include <socket.h>
#include <typedefs.h>

#define CWR (1 << 7)
#define ECE (1 << 6)
#define URG (1 << 5)
#define ACK (1 << 4)
#define PSH (1 << 3)
#define RST (1 << 2)
#define SYN (1 << 1)
#define FIN (1 << 0)

#define TCP_STATE_CLOSED 0
#define TCP_STATE_LISTEN 1
#define TCP_STATE_SYN_SENT 2
#define TCP_STATE_SYN_RECIEVED 3
#define TCP_STATE_ESTABLISHED 4
#define TCP_STATE_CLOSE_WAIT 5
#define TCP_STATE_FIN_WAIT1 6
#define TCP_STATE_CLOSING 7
#define TCP_STATE_LAST_ACK 8
#define TCP_STATE_FIN_WAIT2 9
#define TCP_STATE_TIME_WAIT 10

void handle_tcp(ipv4_t src_ip, ipv4_t dst_ip, const u8 *payload,
                u32 payload_length);
int send_tcp_packet(struct TcpConnection *con, const u8 *payload,
                    u16 payload_length);
void tcp_close_connection(struct TcpConnection *con);
u16 tcp_can_send(struct TcpConnection *con);
void tcp_send_empty_payload(struct TcpConnection *con, u8 flags);
