#include <math.h>
#include <sched/scheduler.h>
#include <syscalls.h>

char *syscall_getcwd(char *buf, size_t size) {
  kprintf("syscall_getcwd\n");
  const char *cwd = get_current_task()->current_working_directory;
  size_t len = min(size, strlen(cwd));
  strlcpy(buf, get_current_task()->current_working_directory, len);
  return buf;
}
