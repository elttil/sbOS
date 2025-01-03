#include <errno.h>
#include <sys/mman.h>
#include <syscall.h>

extern int errno;

void *mmap(void *addr, size_t length, int prot, int flags, int fd,
           size_t offset) {
  SYS_MMAP_PARAMS args = {
      .addr = addr,
      .length = length,
      .prot = prot,
      .flags = flags,
      .fd = fd,
      .offset = offset,
  };
  //  return (void*)syscall(SYS_MMAP, &args, 0, 0, 0, 0);
  RC_ERRNO(syscall(SYS_MMAP, &args, 0, 0, 0, 0));
}
