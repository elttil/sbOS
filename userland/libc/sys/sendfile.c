#include <sys/sendfile.h>
#include <syscall.h>
#include <errno.h>

u32 sendfile(int out_fd, int in_fd, off_t *offset, size_t count,
             int *error_rc) {
  u32 r = syscall(SYS_SENDFILE, out_fd, in_fd, offset, count, error_rc);
  if (error_rc) {
    errno = -1 * *error_rc;
  }
  return r;
}
