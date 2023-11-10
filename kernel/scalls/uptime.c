#include <scalls/uptime.h>
#include <drivers/pit.h>

u32 syscall_uptime(void) { return (u32)pit_num_ms(); }
