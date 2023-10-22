#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct s_cookie {
  char *s;
};

size_t s_write(FILE *f, const unsigned char *s, size_t l) {
  struct s_cookie *c = f->cookie;
  memcpy(c->s, s, l);
  c->s += l;
  *(c->s) = '\0';
  return l;
}

int vsprintf(char *str, const char *format, va_list ap) {
  struct s_cookie c = {.s = str};
  FILE f = {
      .write = s_write,
      .cookie = &c,
  };
  return vfprintf(&f, format, ap);
}

int sprintf(char *str, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int rc = vsprintf(str, format, ap);
  va_end(ap);
  return rc;
}
