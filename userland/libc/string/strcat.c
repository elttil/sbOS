#include <string.h>

char *strcat(char *s1, const char *s2) {
  strcpy(s1 + strlen(s1), s2);
  return s1;
  /*
char *r = s1;
for (; *s1; s1++)
;
for (; *s2; s2++, s1++)
*s1 = *s2;
return r;*/
}
