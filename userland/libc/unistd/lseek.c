#include <errno.h>
#include <unistd.h>
#include <syscall.h>

off_t lseek(int fildes, off_t offset, int whence) {
  RC_ERRNO(syscall(SYS_LSEEK, (u32)fildes, offset, whence, 0, 0));
}
