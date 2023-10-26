#include <stdint.h>
void get_mac_address(uint8_t mac[6]);
void rtl8139_init(void);
int rtl8139_send_data(uint8_t *data, uint16_t data_size);
