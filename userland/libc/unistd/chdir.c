#include <syscall.h>
#include <unistd.h>

int chdir(const char *path) {
  RC_ERRNO(syscall(SYS_CHDIR, path, 0, 0, 0, 0));
}
