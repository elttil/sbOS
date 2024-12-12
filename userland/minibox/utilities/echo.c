#include "include.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

int echo(char **str, int new_line) {
  for (; *str;) {
    printf("%s", *str);
    if (*++str) {
      putchar(' ');
    }
  }

  if (new_line) {
    putchar('\n');
  }
  return 0;
}

int echo_main(int argc, char **argv) {
  int new_line = 1;

  for (; *++argv && '-' == **argv;) {
    switch (*(*argv + 1)) {
    case 'n':
      new_line = 0;
      break;
    }
  }

  return echo(argv, new_line);
}
