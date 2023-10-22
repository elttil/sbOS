#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

int cmdfd;

void execsh(void) {
  char *argv[] = {NULL};
  execv("/sh", argv);
}

void newtty(void) {
  int m;
  int s;
  openpty(&m, &s, NULL, NULL, NULL);
  int pid = fork();
  if (0 == pid) {
    dup2(s, 0);
    dup2(s, 1);
    dup2(s, 2);
    execsh();
    return;
  }
  close(s);
  cmdfd = m;
}

void shell() {
  struct pollfd fds[2];
  fds[0].fd = cmdfd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;
  fds[1].fd = open("/dev/keyboard", O_RDONLY, 0);
  fds[1].events = POLLIN;
  fds[1].revents = 0;
  for (;; fds[0].revents = fds[1].revents = 0) {
    poll(fds, 2, 0);
    if (fds[0].revents & POLLIN) {
      char c[4096];
      int rc = read(fds[0].fd, c, 4096);
      assert(rc > 0);
      for (int i = 0; i < rc; i++)
        putchar(c[i]);
    }
    if (fds[1].revents & POLLIN) {
      char c;
      int rc = read(fds[1].fd, &c, 1);
      assert(rc > 0);
      write(cmdfd, &c, 1);
      //      putchar(c);
    }
  }
}

int init_main(void) {
  if (1 != getpid()) {
    printf("minibox init must be launched as pid1.\n");
    return 1;
  }
  if (fork())
    for (;;)
      wait(NULL);
  char *a[] = {NULL};
  execv("/ws", a);
  return 1;
}
