#include <drivers/pst.h>
#include <fs/tmpfs.h>

int openpty(int *amaster, int *aslave, char *name,
            /*const struct termios*/ void *termp,
            /*const struct winsize*/ void *winp) {
	(void)name;
	(void)termp;
	(void) winp;
  int fd[2];
  pipe(fd); // This depends upon that pipe will support read and write
            // through the same fd. In reality this should not be the
            // case.
  *amaster = fd[0];
  *aslave = fd[1];
  return 0;
}
