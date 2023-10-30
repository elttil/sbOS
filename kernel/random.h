#include <stdint.h>
#include <fs/vfs.h>
#include <defs.h>

void setup_random(void);
void add_random_devices(void);
void get_random(uint8_t* buffer, uint64_t len);
