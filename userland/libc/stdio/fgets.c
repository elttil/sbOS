#include <stdio.h>

char *fgets(char *s, int n, FILE *stream) {
  for (int i = 0; i < n; i++) {
    char c;
    fread(&c, 1, 1, stream);
    if (stream->has_error) {
      return NULL;
    }
    if (stream->is_eof) {
      return NULL;
    }
    s[i] = c;
    if (c == '\n') {
      break;
    }
  }
  return s;
}
