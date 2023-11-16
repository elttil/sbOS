#include <syscalls.h>
#include <sched/scheduler.h>
#include <signal.h>

int syscall_kill(pid_t pid, int sig) {return kill(pid, sig);}
