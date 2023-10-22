#include <string.h>

char *strcat(char *s1, const char *s2) {
  strcpy(s1 + strlen(s1), s2);
  return s1;
}
