#ifndef SYS_SHM_H
#define SYS_SHM_H
#include <stddef.h>
#include <stdint.h>
typedef int mode_t;

typedef struct SYS_SHM_OPEN_PARAMS {
  const char *name;
  int oflag;
  mode_t mode;
} __attribute__((packed)) SYS_SHM_OPEN_PARAMS;

typedef struct SYS_FTRUNCATE_PARAMS {
  int fildes;
  uint64_t length;
} __attribute__((packed)) SYS_FTRUNCATE_PARAMS;

int syscall_shm_open(SYS_SHM_OPEN_PARAMS *args);
int syscall_ftruncate(SYS_FTRUNCATE_PARAMS *args);
#endif
