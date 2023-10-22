#include <stdlib.h>

int system(const char *command) {
  if (!command)
    return NULL;
  int pid = fork();
  if (0 == pid) {
    char *argv[2];
    argv[0] = "/sh";
    argv[1] = command;
    execv("/sh", argv);
  }
  // FIXME: Use waitpid
  int rc;
  (void)wait(&rc);
  return rc;
}
