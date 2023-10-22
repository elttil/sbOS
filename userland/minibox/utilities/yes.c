#include "include.h"
#include <stdio.h>

int yes_main(int argc, char **argv) {
  if (argc < 2)
    for (;;)
      puts("y");

  for (;; putchar('\n'))
    for (int i = 1; i < argc; i++)
      printf("%s ", argv[i]);
  return 0;
}
