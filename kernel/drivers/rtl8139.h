#include <typedefs.h>
void get_mac_address(u8 mac[6]);
void rtl8139_init(void);
void rtl8139_send_data(u8 *data, u16 data_size);
u8 *nic_get_buffer(void);
void nic_send_buffer(u16 data_size);
