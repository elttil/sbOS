#include <errno.h>
#include <syscall.h>
#include <unistd.h>

int lseek(int fildes, int offset, int whence) {
  RC_ERRNO(syscall(SYS_LSEEK, (u32)fildes, offset, whence, 0, 0));
}
