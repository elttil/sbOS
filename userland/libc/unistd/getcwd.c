#include <syscall.h>
#include <unistd.h>

char *getcwd(char *buf, size_t size) {
  return syscall(SYS_GETCWD, buf, size, NULL, NULL, NULL);
}
