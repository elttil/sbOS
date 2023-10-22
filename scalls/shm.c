#include "shm.h"
#include "../fs/shm.h"

int syscall_shm_open(SYS_SHM_OPEN_PARAMS *args) {
  return shm_open(args->name, args->oflag, args->mode);
}

int syscall_ftruncate(SYS_FTRUNCATE_PARAMS *args) {
  return ftruncate(args->fildes, args->length);
}
