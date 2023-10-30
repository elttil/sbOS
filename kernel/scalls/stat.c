#include <errno.h>
#include <fs/vfs.h>
#include <scalls/stat.h>

int syscall_stat(SYS_STAT_PARAMS *args) {
  const char *pathname = copy_and_allocate_user_string(args->pathname);
  struct stat *statbuf = args->statbuf;
  vfs_inode_t *i = vfs_internal_open(pathname);
  if (!i)
    return -ENOENT;
  statbuf->st_size = i->file_size;
  return 0;
}
