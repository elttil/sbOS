#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern int errno;
extern int get_value(char c, long base);

long ftnum(FILE *stream, int base, int *error) {
  char c;
  long ret_value = 0;
  *error = 0;
  // Ignore inital white-space sequence
  for (;;) {
    if (EOF == (c = fgetc(stream))) {
      *error = 1;
      return 0;
    }

    if (!isspace(c)) {
      ungetc(c, stream);
      break;
    }
  }
  if (c == '\0') {
    *error = 1;
    return 0;
  }
  if (!isdigit(c)) {
    *error = 1;
    return 0;
  }
  if (!(2 <= base && 36 >= base)) {
    *error = 1;
    return 0;
  }
  for (;;) {
    if (EOF == (c = fgetc(stream))) {
      break;
    }
    if (c == '\0') {
      ungetc(c, stream);
      break;
    }
    int val = get_value(c, base);
    if (-1 == val) {
      ungetc(c, stream);
      break;
    }
    if (ret_value * base > LONG_MAX - val) {
      ungetc(c, stream);
      errno = ERANGE;
      *error = 1;
      return 0;
    }
    ret_value *= base;
    ret_value += val;
  }
  return ret_value;
}

int vfscanf(FILE *stream, const char *format, va_list ap) {
  int rc = 0; // Upon successful completion, these functions shall return the
              // number of successfully matched and assigned input items
  int cont = 0;
  int suppress = 0;
  for (; *format; format++) {
    if (*format != '%' && !cont) {
      char c;
      if (isspace(*format)) {
        continue;
      }
      if (EOF == (c = fgetc(stream))) {
        break;
      }
      if (*format == c) { // TODO: Make sure this is the correct behaviour
        continue;
      }
      // TODO: Make sure this is the correct behaviour
      errno = EINVAL;
      assert(0);
      break;
    }

    if (*format == '%' && !cont) {
      cont = 1;
      continue;
    }

    int is_long = 0;
    switch (*format) {
    case 'l':
      is_long++;
      assert(is_long < 3);
      cont = 1;
      break;
    case 'i': // Matches an optionally signed integer, whose format is the same
              // as expected for the subject sequence of strtol() with 0 for the
              // base argument.
    case 'd': {
      // Matches an optionally signed decimal integer, whose format is the
      // same as expected for the subject sequence of strtol() with the value
      // 10 for the base argument. In the absence of a size modifier, the
      // application shall ensure that the corresponding argument is a pointer
      // to int.
      int err = 0;
      int result = ftnum(stream, 10, &err);
      if (err) {
        cont = 0;
        break;
      }
      if (!suppress) {
        if (2 == is_long) {
          *((long long *)va_arg(ap, long long *)) = result;
        } else if (1 == is_long) {
          *((long *)va_arg(ap, long *)) = result;
        } else {
          *((int *)va_arg(ap, int *)) = result;
        }
        rc++;
      }
      assert(0 == err);
      cont = 0;
      break;
    }
    case 'c': {
      char result = fgetc(stream);
      if (!suppress) {
        *((char *)va_arg(ap, char *)) = result;
        rc++;
      }
      cont = 0;
      break;
    }
    case '*': // Assignment suppression
      suppress = 1;
      cont = 1;
      break;
    default:
      printf("vfscanf: Got %c but not supported.\n", *format);
      assert(0);
      break;
    }
    if (!cont) {
      suppress = 0;
    }
  }
  return rc;
}

struct sscanf_cookie {
  const char *s;
  unsigned long offset;
};

size_t sscanf_read(FILE *f, unsigned char *s, size_t l) {
  struct sscanf_cookie *c = f->cookie;
  if (!*(c->s + c->offset)) {
    return 0;
  }
  size_t r = 0;
  for (; l && *(c->s); l--, c->offset += 1) {
    *s = *(c->s + c->offset);
    s++;
    r++;
  }
  if (!(*(c->s + c->offset))) {
    f->is_eof = 1;
  }
  /*
  memcpy(s, c->s, l);
  c->s += l;*/
  return r;
}

int sscanf_seek(FILE *stream, long offset, int whence) {
  struct sscanf_cookie *c = stream->cookie;
  off_t ret_offset = c->offset;
  switch (whence) {
  case SEEK_SET:
    // TODO: Avoid running past the pointer
    // Should that even be checked?
    ret_offset = offset;
    break;
  case SEEK_CUR:
    ret_offset += offset;
    break;
  case SEEK_END:
    for (; *(c->s + ret_offset); ret_offset++)
      ;
    break;
  default:
    return -EINVAL;
    break;
  }
  c->offset = ret_offset;
  return ret_offset;
}

int vsscanf(const char *s, const char *restrict format, va_list ap) {
  struct sscanf_cookie c = {.s = s, .offset = 0};
  FILE f = {
      .read = sscanf_read,
      .seek = sscanf_seek,
      .cookie = &c,
      .has_buffered_char = 0,
      .is_eof = 0,
      .has_error = 0,
      .offset_in_file = 0,
  };
  return vfscanf(&f, format, ap);
}

int sscanf(const char *s, const char *restrict format, ...) {
  va_list ap;
  va_start(ap, format);
  int rc = vsscanf(s, format, ap);
  va_end(ap);
  return rc;
}
