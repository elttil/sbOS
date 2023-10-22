#include <stdio.h>

int fputs(const char *s, FILE *stream) {
  const char *b = s;
  for (; *s; s++)
    if (0 == fwrite(s, 1, 1, stream))
      return EOF;
  return s - b;
}
