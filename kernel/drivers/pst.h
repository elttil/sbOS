#ifndef PST_H
#define PST_H
#include "../fs/vfs.h"

int openpty(int *amaster, int *aslave, char *name, /*const struct termios*/ void *termp,
            /*const struct winsize*/ void *winp);
#endif
