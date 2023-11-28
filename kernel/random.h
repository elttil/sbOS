#include <defs.h>
#include <fs/vfs.h>
#include <typedefs.h>

void setup_random(void);
void add_random_devices(void);
void get_random(u8 *buffer, u64 len);
void get_fast_insecure_random(u8 *buffer, u64 len);
