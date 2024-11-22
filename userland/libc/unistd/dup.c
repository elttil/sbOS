#include <syscall.h>
#include <unistd.h>

int dup(int fildes) {
  RC_ERRNO(syscall(SYS_DUP, (u32)fildes, 0, 0, 0, 0));
}
