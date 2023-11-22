#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *tmpnam(char *s) {
  assert(!s);
  s = malloc(100);
  strcpy(s, "/tmp.XXXXXX");
  mkstemp(s);
  return s;
}
