#include <fcntl.h>
#include <stdlib.h>

char rand_char(void) {
  return 'A' + (rand() % 10);
}

int mkstemp(char *template) {
  // FIXME: Incomplete
  const char *s = template;
  for (; *template; template ++) {
    if ('X' == *template) {
      *template = rand_char();
    }
  }
  return open(s, O_RDWR, O_CREAT);
}
