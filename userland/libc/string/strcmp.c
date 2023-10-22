#include <string.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/
int strcmp(const char *s1, const char *s2) {
  // The strcmp() function shall compare the string pointed to by s1
  // to the string pointed to by s2.
  int l1, l2, rc;
  l1 = l2 = rc = 0;
  for (; *s1 || *s2;) {
    if (*s1 != *s2)
      rc++;
    if (*s1) {
      l1++;
      s1++;
    }
    if (*s2) {
      l2++;
      s2++;
    }
  }

  // Upon completion, strcmp() shall return an integer greater than,
  // equal to, or less than 0, if the string pointed to by s1 is
  // greater than, equal to, or less than the string pointed to by
  // s2, respectively.
  if (l2 > l1)
    return -rc;
  return rc;
}
