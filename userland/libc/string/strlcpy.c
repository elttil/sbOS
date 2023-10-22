#include <string.h>

// Copy string s2 to buffer s1 of size n.  At most n-1
// chars will be copied.  Always NUL terminates (unless n == 0).
// Returns strlen(s2); if retval >= n, truncation occurred.
size_t *strlcpy(char *s1, const char *s2, size_t n) {
  size_t tmp_n = n;
  const char *os2 = s2;
  for (; tmp_n; tmp_n--) {
    if ((*s1++ = *s2++) == '\0')
      break;
  }
  if (tmp_n == 0) {
    if (n != 0)
      *s1 = '\0'; /* NUL-terminate s1 */
    while (*s2++)
      ;
  }
  return s2 - os2 - 1;
}
