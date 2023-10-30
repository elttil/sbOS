#include <fs/vfs.h>
#include <scalls/ftruncate.h>

int syscall_ftruncate(int fd, size_t length) {
  return vfs_ftruncate(fd, length);
}
