#include "fifo.h"
#include <assert.h>
#include <errno.h>

#define STARTING_SIZE 4096

void fifo_close(vfs_fd_t *fd) {
  // TODO: Implement
  (void)fd;
  return;
}

void fifo_realloc(FIFO_FILE *file) {
  file->buffer_len += 4096;
  file->buffer = krealloc(file->buffer, file->buffer_len);
}

int fifo_object_write(u8 *buffer, u64 offset, u64 len, FIFO_FILE *file) {
  (void)offset;
  file->has_data = 1;
  if (file->write_len + len >= file->buffer_len) {
    file->can_write = 0;
    fifo_realloc(file);
    return -EAGAIN;
  }
  memcpy(file->buffer + file->write_len, buffer, len);
  file->write_len += len;
  return len;
}

int fifo_object_read(u8 *buffer, u64 offset, u64 len, FIFO_FILE *file) {
  (void)offset;
  if (file->write_len == 0) {
    file->has_data = 0;
    return -EAGAIN;
  }

  if (len == 0) {
    return 0;
  }

  file->can_write = 1;
  if (len > file->write_len) {
    len = file->write_len;
  }

  memcpy(buffer, file->buffer, len);
  // Shift bufffer to the left
  memcpy(file->buffer, file->buffer + len, file->buffer_len - len);

  file->write_len -= len;
  if (file->write_len == 0) {
    file->has_data = 0;
  }
  return len;
}

FIFO_FILE *create_fifo_object(void) {
  FIFO_FILE *n = kmalloc(sizeof(FIFO_FILE));
  n->buffer = kmalloc(STARTING_SIZE);
  n->buffer_len = STARTING_SIZE;
  n->write_len = 0;
  return n;
}

int fifo_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  (void)offset;
  FIFO_FILE *file = (FIFO_FILE *)fd->inode->internal_object;
  int rc = fifo_object_write(buffer, offset, len, file);
  fd->inode->has_data = file->has_data;
  fd->inode->can_write = file->can_write;
  return rc;
}

int fifo_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  FIFO_FILE *file = (FIFO_FILE *)fd->inode->internal_object;
  file->is_blocking = !(fd->flags & O_NONBLOCK);
  int rc = fifo_object_read(buffer, offset, len, file);
  fd->inode->has_data = file->has_data;
  fd->inode->can_write = file->can_write;
  return rc;
}
