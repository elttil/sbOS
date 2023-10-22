#include <stdio.h>
#include <unistd.h>

int putchar(int c) {
  printf("%c", (char)c);
  return c;
}
