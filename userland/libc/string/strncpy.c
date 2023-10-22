#include <string.h>

char *strncpy(char *s1, const char *s2, size_t n) {
  char *rc = s1;
  for (; n > 0; s1++, s2++, n--) {
    *s1 = *s2;
    if (!*s2)
      break;
  }
  for (; n > 0; n--,s1++)
    *s1 = '\0';
  return rc;
}
