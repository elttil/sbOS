#include <mmu.h>
#include <syscalls.h>

u32 syscall_map_frames(u32 address, u32 size) {
  return (u32)mmu_map_user_frames((void *)address, size);
}
