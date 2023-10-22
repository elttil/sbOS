#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
  if (fork())
    for (;;)
      wait(NULL);

  char *a[] = {NULL};
  execv("/term", a);
  return 1;
}
