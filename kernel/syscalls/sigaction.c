#include <sched/scheduler.h>
#include <signal.h>
#include <syscalls.h>

int syscall_sigaction(int sig, const struct sigaction *restrict act,
                      struct sigaction *restrict oact) {
  set_signal_handler(sig, act->sa_handler);
  return 0;
}
