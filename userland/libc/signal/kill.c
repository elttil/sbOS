#include <signal.h>
#include <syscall.h>

int kill(pid_t pid, int sig) { RC_ERRNO(syscall(SYS_KILL, pid, sig, 0, 0, 0)) }
