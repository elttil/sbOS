#include <signal.h>
#include <syscall.h>

int sigaction(int sig, const struct sigaction *act,
              struct sigaction *oact) {
  RC_ERRNO(syscall(SYS_SIGACTION, sig, act, oact, 0,0))
}
