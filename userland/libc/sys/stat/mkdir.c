#include <sys/stat.h>
#include <syscall.h>

int mkdir(const char *path, mode_t mode) {
  return syscall(SYS_MKDIR, (void *)path, (void *)mode, 0, 0, 0);
}
