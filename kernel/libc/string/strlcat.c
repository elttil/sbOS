#include <string.h>

size_t strlcat(char *dst, const char *src, size_t dsize) {
  const char *odst = dst;
  const char *osrc = src;
  size_t n = dsize;
  size_t dlen;

  for (; n-- > 0 && '\0' != *dst;) {
    dst++;
  }
  dlen = dst - odst;
  n = dsize - dlen;

  n--;
  if (0 == n) {
    return (dlen + strlen(src));
  }

  for (; '\0' != *src;) {
    if (n > 0) {
      *dst++ = *src;
      n--;
    }
    src++;
  }
  *dst = '\0';

  return (dlen + (src - osrc)); /* count does not include NUL */
}
