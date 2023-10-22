#include <scalls/uptime.h>
#include <drivers/pit.h>

uint32_t syscall_uptime(void) { return (uint32_t)pit_num_ms(); }
