#include <stddef.h>
#include <string.h>

// Copy string src to buffer dst of size dsize.  At most dsize-1
// chars will be copied.  Always NUL terminates (unless dsize == 0).
// Returns strlen(src); if retval >= dsize, truncation occurred.
size_t strlcpy(char *dst, const char *src, size_t dsize) {
  size_t n = dsize;
  const char *osrc = src;
  for (; n; n--) {
    if ((*dst++ = *src++) == '\0')
      break;
  }
  if (n == 0) {
    if (dsize != 0)
      *dst = '\0'; /* NUL-terminate dst */
    while (*src++)
      ;
  }
  return src - osrc - 1;
}
