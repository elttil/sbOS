#include <unistd.h>
#include <syscall.h>
#include <sys/types.h>

pid_t getpid(void) {
    return s_syscall(SYS_GETPID);
}
