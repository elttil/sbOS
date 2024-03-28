// kill
#include "include.h"
#include <signal.h>
#include <stdio.h>

int kill_main(int argc, char **argv) {
  kill(4, SIGTERM);
  return 0;
}
