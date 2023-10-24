#include <syscall.h>
#include <unistd.h>

int ftruncate(int fildes, size_t length) {
  RC_ERRNO(syscall(SYS_FTRUNCATE, fildes, length, 0, 0, 0));
}
