#include <mmu.h>
#include <syscalls.h>

u32 syscall_virtual_to_physical(u32 virtual) {
  return (u32)virtual_to_physical((void *)virtual, NULL);
}
