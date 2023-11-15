#include <types.h>
#include <time.h>

typedef struct SYS_STAT_PARAMS {
  const char *pathname;
  struct stat *statbuf;
} __attribute__((packed)) SYS_STAT_PARAMS;

int syscall_stat(SYS_STAT_PARAMS *args);
