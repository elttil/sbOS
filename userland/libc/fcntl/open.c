#include <fcntl.h>
#include <stdarg.h>
#include <syscall.h>

int open(const char *file, int flags, ...) {
  mode_t mode = 0;

  if (flags & O_CREAT) {
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);
  }
  RC_ERRNO(syscall(SYS_OPEN, file, flags, mode, 0, 0));
}
