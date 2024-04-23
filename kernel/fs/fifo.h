typedef struct S_FIFO_FILE FIFO_FILE;
#ifndef FIFO_H
#define FIFO_H
#include "vfs.h"
#include <stddef.h>
#include <typedefs.h>

struct S_FIFO_FILE {
  char *buffer;
  u64 buffer_len;
  u64 write_len;
  u8 is_blocking;
  u8 has_data;
  u8 can_write;
};

FIFO_FILE *create_fifo_object(void);
int fifo_object_write(u8 *buffer, u64 offset, u64 len, FIFO_FILE *file);
int fifo_object_read(u8 *buffer, u64 offset, u64 len, FIFO_FILE *file);
int fifo_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd);
int fifo_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd);
#endif
