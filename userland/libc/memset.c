#include <stddef.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/
void *memset(void *s, int c, size_t n) {
  // The memset() function shall copy c (converted to an unsigned
  // char) into each of the first n bytes of the object pointed to by
  // s.

  unsigned char *p = s;
  for (; n > 0; n--, p++) {
    *p = (unsigned char)c;
  }

  // The memset() function shall return s
  return s;
}
