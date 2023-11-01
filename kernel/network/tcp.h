void handle_tcp(uint8_t src_ip[4], const uint8_t *payload,
                uint32_t payload_length);
void send_tcp_packet(struct INCOMING_TCP_CONNECTION *inc, uint8_t *payload,
                     uint16_t payload_length);
void tcp_close_connection(struct INCOMING_TCP_CONNECTION *s);
