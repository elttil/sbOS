#include <signal.h>
int syscall_sigaction(int sig, const struct sigaction *restrict act,
              struct sigaction *restrict oact) ;
