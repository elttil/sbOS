#include <syscalls.h>

// FIXME: PERMISSION CHECKS
void syscall_outw(u16 port, u16 word) {
  outw(port, word);
}

void syscall_outl(u16 port, u32 l) {
  outl(port, l);
}

u32 syscall_inl(u16 port) {
  return inl(port);
}
