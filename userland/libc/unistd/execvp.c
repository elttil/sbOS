#include <unistd.h>
#include <syscall.h>

// FIXME: Path resolution
int execvp(const char *file, char *const argv[]) {
  struct SYS_EXEC_PARAMS args = {.path = file, .argv = argv};
  return syscall(SYS_EXEC, &args, 0, 0, 0, 0);
}
