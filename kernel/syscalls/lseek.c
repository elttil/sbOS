#include <assert.h>
#include <errno.h>
#include <fs/vfs.h>

// FIXME: These should be in a shared header file with libc
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

off_t syscall_lseek(int fd, off_t offset, int whence) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd, NULL);
  if (!fd_ptr) {
    return -EBADF;
  }

  off_t ret_offset = fd_ptr->offset;
  switch (whence) {
  case SEEK_SET:
    ret_offset = offset;
    break;
  case SEEK_CUR:
    ret_offset += offset;
    break;
  case SEEK_END:
    assert(fd_ptr->inode);
    ret_offset = fd_ptr->inode->file_size + offset;
    break;
  default:
    return -EINVAL;
    break;
  }
  fd_ptr->offset = ret_offset;
  return ret_offset;
}
