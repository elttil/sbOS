#ifndef PTY_H
#define PTY_H
int openpty(int *amaster, int *aslave, char *name,
                    /*const struct termios*/ void *termp,
                    /*const struct winsize*/ void *winp);
#endif
