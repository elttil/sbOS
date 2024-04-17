#include <stdio.h>
#include <unistd.h>

int putchar(int c) {
  write(1, (char *)&c, 1);
  return c;
}
