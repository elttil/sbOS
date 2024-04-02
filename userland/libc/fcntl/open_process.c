#include <fcntl.h>
#include <syscall.h>

int open_process(int pid) {
  RC_ERRNO(syscall(SYS_OPEN_PROCESS, pid, 0, 0, 0, 0));
}
