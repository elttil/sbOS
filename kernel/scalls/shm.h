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

int syscall_shm_open(SYS_SHM_OPEN_PARAMS *args);
#endif
