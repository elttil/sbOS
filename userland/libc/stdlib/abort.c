#include <assert.h>
#include <stdlib.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/abort.html
void abort(void) {
  printf("aborting!!!!\n");
  assert(0);
  for (;;)
    ;
}
