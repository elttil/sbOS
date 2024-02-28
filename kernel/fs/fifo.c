#include "fifo.h"
#include <errno.h>

#define STARTING_SIZE 4096

void fifo_close(vfs_fd_t *fd) {
  // TODO: Implement
  (void)fd;
  return;
}

int fifo_object_write(u8 *buffer, u64 offset, u64 len, FIFO_FILE *file) {
  (void)offset;
  file->has_data = 1;
  if (file->write_len + len >= file->buffer_len) {
    file->can_write = 0;
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

int create_fifo(void) {
  int fd_n = 0;
  for (; get_current_task()->file_descriptors[fd_n]; fd_n++)
    ;

  vfs_fd_t *fd = kmalloc(sizeof(vfs_fd_t));
  fd->flags = O_RDWR | O_NONBLOCK;
  get_current_task()->file_descriptors[fd_n] = fd;
  fd->inode = kmalloc(sizeof(vfs_inode_t));

  fd->inode->internal_object = (void *)create_fifo_object();
  fd->inode->open = NULL;
  fd->inode->read = fifo_read;
  fd->inode->write = fifo_write;
  fd->inode->close = fifo_close;
  fd->inode->get_vm_object = NULL;
  fd->inode->is_open = 1;

  return fd_n;
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
