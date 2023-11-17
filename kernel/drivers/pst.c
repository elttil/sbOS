#include <drivers/pst.h>
#include <fs/tmpfs.h>
#include <fs/vfs.h>

int openpty(int *amaster, int *aslave, char *name,
            /*const struct termios*/ void *termp,
            /*const struct winsize*/ void *winp) {
  (void)name;
  (void)termp;
  (void)winp;
  int fd[2];
  pipe(fd); // This depends upon that pipe will support read and write
            // through the same fd. In reality this should not be the
            // case.
  get_vfs_fd(fd[0])->is_tty = 1;
  get_vfs_fd(fd[1])->is_tty = 1;
  *amaster = fd[0];
  *aslave = fd[1];
  return 0;
}
