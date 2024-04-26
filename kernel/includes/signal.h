#ifndef SIGNAL_H
#define SIGNAL_H
#include <sys/types.h>
#define SIGHUP 0
#define SIGINT 1
#define SIGWINCH 2
#define SIGQUIT 3
#define SIG_IGN 4
#define SIGSEGV 5
#define SIGTERM 15
typedef int sigset_t;

union sigval {
  int sival_int;   // Integer signal value.
  void *sival_ptr; // Pointer signal value.
};

struct siginfo {
  int si_signo;          //  Signal number.
  int si_code;           //   Signal code.
  int si_errno;          //  If non-zero, an errno value associated with
                         // this signal, as described in <errno.h>.
  pid_t si_pid;          //    Sending process ID.
  uid_t si_uid;          //    Real user ID of sending process.
  void *si_addr;         //   Address of faulting instruction.
  int si_status;         // Exit value or signal.
  long si_band;          //   Band event for SIGPOLL.
  union sigval si_value; //  Signal value.
};

typedef struct siginfo siginfo_t;

int kill(pid_t pid, int sig);

struct sigaction {
  void (*sa_handler)(int); // Pointer to a signal-catching function or one of
                           // the macros SIG_IGN or SIG_DFL.
  sigset_t sa_mask; // Additional set of signals to be blocked during execution
                    // of signal-catching function.
  int sa_flags;     // Special flags to affect behavior of signal.
  void (*sa_sigaction)(int, siginfo_t *,
                       void *); // Pointer to a signal-catching function.
};

int sigaction(int sig, const struct sigaction *act, struct sigaction *oact);
#endif // SIGNAL_H
