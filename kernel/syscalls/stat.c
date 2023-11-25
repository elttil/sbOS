#include <errno.h>
#include <fs/vfs.h>
#include <syscalls.h>

int syscall_stat(SYS_STAT_PARAMS *args) {
  const char *pathname = copy_and_allocate_user_string(args->pathname);
  struct stat *statbuf = args->statbuf;
  int fd = vfs_open(pathname, O_READ, 0);
  if(0 > fd)
	  return -ENOENT;
  int rc = vfs_fstat(fd, statbuf);
  vfs_close(fd);
  return rc;
}
