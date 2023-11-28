#include <drivers/pit.h>
#include <syscalls.h>

u32 syscall_uptime(void) {
  return (u32)pit_num_ms();
}
