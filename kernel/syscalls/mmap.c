#include <sched/scheduler.h>
#include <syscalls.h>

void *syscall_mmap(SYS_MMAP_PARAMS *args) {
  return mmap(args->addr, args->length, args->prot, args->flags, args->fd,
              args->offset);
}
