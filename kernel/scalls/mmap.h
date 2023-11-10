#include <stddef.h>
#include <typedefs.h>

typedef struct SYS_MMAP_PARAMS {
  void *addr;
  size_t length;
  int prot;
  int flags;
  int fd;
  size_t offset;
} __attribute__((packed)) SYS_MMAP_PARAMS;

void *syscall_mmap(SYS_MMAP_PARAMS *args);
