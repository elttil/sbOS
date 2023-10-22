#include "include.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

int pwd_main(int argc, char **argv) {
  char cwd[PATH_MAX];
  COND_PERROR_EXP(NULL == (getcwd(cwd, PATH_MAX)), "getcwd", return 1);
  puts(cwd);
  return 0;
}
