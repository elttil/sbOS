#include <stdio.h>
#include <stdlib.h>

size_t getdelim(char **lineptr, size_t *n, int delimiter, FILE *stream) {
  if (NULL == *lineptr) {
    *lineptr = malloc(256);
    *n = 256;
  }
  size_t s = 0;
  for (;;) {
    char c;
    if (0 == fread(&c, 1, 1, stream)) {
      s++;
      break;
    }
    if (c == delimiter) {
      break;
    }
    if (s + 1 >= *n) {
      *n += 256;
      *lineptr = realloc(*lineptr, *n);
    }
    (*lineptr)[s] = c;
    s++;
  }
  (*lineptr)[s] = '\0';
  return s;
}
