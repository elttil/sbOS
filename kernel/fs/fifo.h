typedef struct S_FIFO_FILE FIFO_FILE;
#ifndef FIFO_H
#define FIFO_H
#include "vfs.h"
#include <stddef.h>
#include <stdint.h>

struct S_FIFO_FILE {
  char *buffer;
  uint64_t buffer_len;
  uint64_t write_len;
  uint8_t is_blocking;
  uint8_t has_data;
  uint8_t can_write;
};

int create_fifo(void);
FIFO_FILE *create_fifo_object(void);
int fifo_object_write(uint8_t *buffer, uint64_t offset, uint64_t len,
                      FIFO_FILE *file);
int fifo_object_read(uint8_t *buffer, uint64_t offset, uint64_t len,
                     FIFO_FILE *file);
int fifo_write(uint8_t *buffer, uint64_t offset, uint64_t len,
               vfs_fd_t *fd);
int fifo_read(uint8_t *buffer, uint64_t offset, uint64_t len,
              vfs_fd_t *fd);
#endif
