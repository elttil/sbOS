#include <string.h>

// copy bytes in memory with overlapping areas
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/memmove.html
void *memmove(void *s1, const void *s2, size_t n) {
  if(0 == n) {
    return s1;
  }
  // Copying takes place as if the n bytes from the object pointed to by s2 are
  // first copied into a temporary array of n bytes that does not overlap the
  // objects pointed to by s1 and s2, and then the n bytes from the temporary
  // array are copied into the object pointed to by s1.
  unsigned char tmp[n];
  memcpy(tmp, s2, n);
  memcpy(s1, tmp, n);
  return s1;
}
