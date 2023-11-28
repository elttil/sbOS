#include <fs/shm.h>
#include <syscalls.h>

int syscall_shm_open(SYS_SHM_OPEN_PARAMS *args) {
  return shm_open(args->name, args->oflag, args->mode);
}
