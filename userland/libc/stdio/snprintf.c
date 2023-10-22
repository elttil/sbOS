#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct sn_cookie {
  char *s;
  size_t n;
};

#define MIN(a, b) ((a) < (b) ? (a) : (b))

size_t sn_write(FILE *f, const unsigned char *s, const size_t l) {
  struct sn_cookie *c = f->cookie;
  size_t k = MIN(l, c->n);
  memcpy(c->s, s, k);
  c->s += k;
  c->n -= k;
  *(c->s) = '\0';
  // Upon successful completion, the snprintf() function shall return the number
  // of bytes that would be written to s had n been sufficiently large excluding
  // the terminating null byte.
  return l;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
  char dummy[1];
  struct sn_cookie c = {.s = (size ? str : dummy), .n = (size ? size - 1 : 0)};
  FILE f = {
      .write = sn_write,
      .cookie = &c,
  };
  return vfprintf(&f, format, ap);
}

int snprintf(char *str, size_t size, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int rc = vsnprintf(str, size, format, ap);
  va_end(ap);
  return rc;
}
