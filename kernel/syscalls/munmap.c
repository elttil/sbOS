#include <sched/scheduler.h>
#include <syscalls.h>

int syscall_munmap(void *addr, size_t length) {
return munmap(addr, length);
}
