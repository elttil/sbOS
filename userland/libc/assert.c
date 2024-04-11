#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int debug_printf(const char *fmt, ...);

void aFailed(char *f, int l) {
  debug_printf("Assert failed\n");
  debug_printf("%s : %d\n", f, l);
  exit(1);
}
