#include <sys/types.h>
#include <syscall.h>
#include <unistd.h>

pid_t getpid(void) {
  return s_syscall(SYS_GETPID);
}
