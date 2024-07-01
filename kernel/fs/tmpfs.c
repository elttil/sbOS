#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fs/fifo.h>
#include <fs/tmpfs.h>
#include <sched/scheduler.h>
#include <typedefs.h>

void tmp_close(vfs_fd_t *fd) {
  fd->inode->is_open = 0;
  ((tmp_inode *)fd->inode->internal_object)->read_inode->is_open = 0;
}

int tmp_has_data(vfs_inode_t *inode) {
  tmp_inode *calling_file = inode->internal_object;
  return calling_file->fifo->has_data;
}

int tmp_can_write(vfs_inode_t *inode) {
  tmp_inode *child_file = inode->internal_object;
  return child_file->fifo->can_write;
}

int tmp_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  if (!fd->inode->is_open) {
    return -EPIPE;
  }
  tmp_inode *calling_file = fd->inode->internal_object;
  tmp_inode *child_file = calling_file->read_inode->internal_object;
  if (child_file->is_closed) {
    return -EPIPE;
  }

  return fifo_object_write(buffer, offset, len, child_file->fifo);
}

int tmp_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  if (!fd->inode->is_open) {
    return -EPIPE;
  }
  tmp_inode *calling_file = fd->inode->internal_object;
  if (calling_file->is_closed) {
    return -EPIPE;
  }

  return fifo_object_read(buffer, offset, len, calling_file->fifo);
}

void dual_pipe(int fd[2]) {
  vfs_fd_t *fd_ptrs[2];
  for (int i = 0; i < 2; i++) {
    tmp_inode *pipe = kmalloc(sizeof(tmp_inode));
    pipe->fifo = create_fifo_object();

    int is_open = 1;
    void *internal_object = pipe;
    vfs_inode_t *inode = vfs_create_inode(
        0 /*inode_num*/, 0 /*type*/, tmp_has_data, tmp_can_write, is_open,
        internal_object, 0 /*file_size*/, NULL /*open*/, NULL /*create_file*/,
        tmp_read, tmp_write, tmp_close, NULL /*create_directory*/,
        NULL /*get_vm_object*/, NULL /*truncate*/, NULL /*stat*/,
        NULL /*send_signal*/, NULL /*connect*/);
    assert(inode);

    fd[i] =
        vfs_create_fd(O_RDWR | O_NONBLOCK, 0, 0 /*is_tty*/, inode, &fd_ptrs[i]);
    assert(-1 != fd[i]);
  }

  vfs_inode_t *f_inode = fd_ptrs[0]->inode;
  vfs_inode_t *s_inode = fd_ptrs[1]->inode;
  tmp_inode *f_pipe = f_inode->internal_object;
  tmp_inode *s_pipe = s_inode->internal_object;
  f_pipe->read_inode = s_inode;
  s_pipe->read_inode = f_inode;
  f_pipe->is_closed = 0;
  s_pipe->is_closed = 0;
}

void pipe(int fd[2]) {
  vfs_fd_t *fd_ptrs[2];
  for (int i = 0; i < 2; i++) {
    tmp_inode *pipe = kmalloc(sizeof(tmp_inode));
    pipe->fifo = create_fifo_object();

    int is_open = 1;
    void *internal_object = pipe;
    vfs_inode_t *inode = vfs_create_inode(
        0 /*inode_num*/, 0 /*type*/, tmp_has_data, tmp_can_write, is_open,
        internal_object, 0 /*file_size*/, NULL /*open*/, NULL /*create_file*/,
        tmp_read, tmp_write, tmp_close, NULL /*create_directory*/,
        NULL /*get_vm_object*/, NULL /*truncate*/, NULL /*stat*/,
        NULL /*send_signal*/, NULL /*connect*/);
    assert(inode);

    fd[i] = vfs_create_fd(O_RDWR, 0, 0 /*is_tty*/, inode, &fd_ptrs[i]);
    assert(-1 != fd[i]);
  }

  vfs_inode_t *f_inode = fd_ptrs[0]->inode;
  vfs_inode_t *s_inode = fd_ptrs[1]->inode;
  tmp_inode *f_pipe = f_inode->internal_object;
  tmp_inode *s_pipe = s_inode->internal_object;
  f_pipe->read_inode = s_inode;
  s_pipe->read_inode = f_inode;
  f_pipe->is_closed = 0;
  s_pipe->is_closed = 0;
}
