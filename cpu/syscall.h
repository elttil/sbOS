#include "idt.h"
#include <stddef.h>
#include <stdint.h>

void syscalls_init(void);

typedef struct SYS_OPEN_PARAMS {
  char *file;
  int flags;
  int mode;
} __attribute__((packed)) SYS_OPEN_PARAMS;

typedef struct SYS_PREAD_PARAMS {
  int fd;
  void *buf;
  size_t count;
  size_t offset;
} __attribute__((packed)) SYS_PREAD_PARAMS;

typedef struct SYS_READ_PARAMS {
  int fd;
  void *buf;
  size_t count;
} __attribute__((packed)) SYS_READ_PARAMS;

typedef struct SYS_PWRITE_PARAMS {
  int fd;
  void *buf;
  size_t count;
  size_t offset;
} __attribute__((packed)) SYS_PWRITE_PARAMS;

typedef struct SYS_WRITE_PARAMS {
  int fd;
  void *buf;
  size_t count;
} __attribute__((packed)) SYS_WRITE_PARAMS;

typedef struct SYS_EXEC_PARAMS {
  char *path;
  char **argv;
} __attribute__((packed)) SYS_EXEC_PARAMS;

typedef struct SYS_WAIT_PARAMS {
  int *status;
} __attribute__((packed)) SYS_WAIT_PARAMS;

typedef struct SYS_DUP2_PARAMS {
  int org_fd;
  int new_fd;
} __attribute__((packed)) SYS_DUP2_PARAMS;

typedef struct SYS_OPENPTY_PARAMS {
  int *amaster;
  int *aslave;
  char *name;
  /*const struct termios*/ void *termp;
  /*const struct winsize*/ void *winp;
} __attribute__((packed)) SYS_OPENPTY_PARAMS;
