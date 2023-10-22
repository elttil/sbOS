#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void aFailed(char *f, int l) {
  printf("Assert failed\n");
  printf("%s : %d\n", f, l);
  exit(1);
}
