#include <assert.h>
#include <errno.h>
#include <fs/fifo.h>
#include <fs/tmpfs.h>
#include <halts.h>
#include <sched/scheduler.h>
#include <typedefs.h>

void tmp_close(vfs_fd_t *fd) {
  fd->inode->is_open = 0;
  ((tmp_inode *)fd->inode->internal_object)->read_inode->is_open = 0;
}

int tmp_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  tmp_inode *calling_file = fd->inode->internal_object;
  tmp_inode *child_file = calling_file->read_inode->internal_object;
  if (child_file->is_closed)
    return -EPIPE;

  int rc = fifo_object_write(buffer, offset, len, child_file->fifo);
  calling_file->read_inode->has_data = child_file->fifo->has_data;
  fd->inode->can_write = child_file->fifo->can_write;
  return rc;
}

int tmp_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  tmp_inode *calling_file = fd->inode->internal_object;
  tmp_inode *child_file = calling_file->read_inode->internal_object;
  if (calling_file->is_closed)
    return -EPIPE;

  int rc = fifo_object_read(buffer, offset, len, calling_file->fifo);
  fd->inode->has_data = calling_file->fifo->has_data;
  calling_file->read_inode->can_write = child_file->fifo->can_write;
  return rc;
}

void dual_pipe(int fd[2]) {
  for (int i = 0; i < 2; i++) {
    tmp_inode *pipe = kmalloc(sizeof(tmp_inode));
    pipe->fifo = create_fifo_object();

    int has_data = 0;
    int can_write = 1;
    int is_open = 1;
    void *internal_object = pipe;
    vfs_inode_t *inode = vfs_create_inode(
        0 /*inode_num*/, 0 /*type*/, has_data, can_write, is_open,
        internal_object, 0 /*file_size*/, NULL /*open*/, NULL /*create_file*/,
        tmp_read, tmp_write, tmp_close, NULL /*create_directory*/,
        NULL /*get_vm_object*/, NULL /*truncate*/, NULL /*stat*/);
    assert(inode);

    vfs_fd_t *fd_ptr;
    fd[i] = vfs_create_fd(O_RDWR | O_NONBLOCK, 0, 0 /*is_tty*/, inode, &fd_ptr);
    assert(-1 != fd[i]);
  }
  vfs_inode_t *f_inode = get_current_task()->file_descriptors[fd[0]]->inode;
  vfs_inode_t *s_inode = get_current_task()->file_descriptors[fd[1]]->inode;
  tmp_inode *f_pipe = f_inode->internal_object;
  tmp_inode *s_pipe = s_inode->internal_object;
  f_pipe->read_inode = s_inode;
  s_pipe->read_inode = f_inode;
  f_pipe->is_closed = 0;
  s_pipe->is_closed = 0;
}

void pipe(int fd[2]) {
  for (int i = 0; i < 2; i++) {
    tmp_inode *pipe = kmalloc(sizeof(tmp_inode));
    pipe->fifo = create_fifo_object();

    int has_data = 0;
    int can_write = 1;
    int is_open = 1;
    void *internal_object = pipe;
    vfs_inode_t *inode = vfs_create_inode(
        0 /*inode_num*/, 0 /*type*/, has_data, can_write, is_open,
        internal_object, 0 /*file_size*/, NULL /*open*/, NULL /*create_file*/,
        tmp_read, tmp_write, tmp_close, NULL /*create_directory*/,
        NULL /*get_vm_object*/, NULL /*truncate*/, NULL /*stat*/);
    assert(inode);

    vfs_fd_t *fd_ptr;
    fd[i] = vfs_create_fd(O_RDWR, 0, 0 /*is_tty*/, inode, &fd_ptr);
    assert(-1 != fd[i]);
  }
  vfs_inode_t *f_inode = get_current_task()->file_descriptors[fd[0]]->inode;
  vfs_inode_t *s_inode = get_current_task()->file_descriptors[fd[1]]->inode;
  tmp_inode *f_pipe = f_inode->internal_object;
  tmp_inode *s_pipe = s_inode->internal_object;
  f_pipe->read_inode = s_inode;
  s_pipe->read_inode = f_inode;
  f_pipe->is_closed = 0;
  s_pipe->is_closed = 0;
}
