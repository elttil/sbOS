#include "pty.h"
#include "syscall.h"

int openpty(int *amaster, int *aslave, char *name,
            /*const struct termios*/ void *termp,
            /*const struct winsize*/ void *winp) {
  SYS_OPENPTY_PARAMS args = {
      .amaster = amaster,
      .aslave = aslave,
      .name = name,
      .termp = termp,
      .winp = winp,
  };
  return syscall(SYS_OPENPTY, &args, 0, 0, 0, 0);
}
