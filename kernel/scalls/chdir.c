#include <fs/vfs.h>
#include <scalls/chdir.h>

int syscall_chdir(const char *path) {
  return vfs_chdir(path);
}
