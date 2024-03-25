#include <sys/mman.h>
#include <syscall.h>

int munmap(void *addr, size_t length) {
  RC_ERRNO(syscall(SYS_MUNMAP, addr, length,0, 0, 0));
}
