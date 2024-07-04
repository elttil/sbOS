#include <defs.h>
#include <fs/vfs.h>
#include <typedefs.h>

void setup_random(void);
void add_random_devices(void);
void get_random(u8 *buffer, u64 len);
void get_fast_insecure_random(u8 *buffer, u64 len);
void random_add_entropy(u8 *buffer, u64 size);
// A interface that is not as expensive to run in comparison to
// random_add_entropy which may take longer.
void random_add_entropy_fast(u8 *buffer, u64 size);
