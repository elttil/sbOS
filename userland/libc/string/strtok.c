#include <string.h>

char *strtok_s;

char *strtok(char *restrict s, const char *restrict sep) {
  if (s) {
    strtok_s = s;
    return strtok(NULL, sep);
  }
  if (!strtok_s) {
    return NULL;
  }
  char *e = strpbrk(strtok_s, sep);
  if (!e) {
    char *r = strtok_s;
    strtok_s = NULL;
    return r;
  }
  *e = '\0';
  e--;
  for (; *sep; sep++) {
    e++;
  }
  e++;
  char *r = strtok_s;
  strtok_s = e;
  return r;
}
