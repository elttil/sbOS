#include <errno.h>
#include <syscalls.h>

int syscall_open(const char *file, int flags, mode_t mode) {
  const char *_file = copy_and_allocate_user_string(file);
  if (!_file) {
    return -EFAULT;
  }
  int _flags = flags;
  int _mode = mode;
  int rc = vfs_open(_file, _flags, _mode);
  kfree((void*)_file);
  return rc;
}
