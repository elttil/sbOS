#include <syscall.h>
#include <unistd.h>

// FIXME: Technically exit and _exit are different but this
// stackoverflow answer says that it does not usually matter. So lets
// hope that is the case.
// https://stackoverflow.com/a/5423108
void _exit(int status) { syscall(SYS_EXIT, (void *)status, 0, 0, 0, 0); }
