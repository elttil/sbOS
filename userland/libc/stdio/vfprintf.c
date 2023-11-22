#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const char HEX_SET[0x10] = {'0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

#define FILE_WRITE(_f, _s, _l, _r)                                             \
  {                                                                            \
    size_t _rc = _f->write(_f, (const unsigned char *)_s, _l);                 \
    if ((size_t)-1 == _rc)                                                     \
      assert(0);                                                               \
    *(int *)(_r) += _rc;                                                       \
  }

int fprint_num(FILE *f, int n, int base, char *char_set, int prefix,
               int zero_padding, int right_padding) {
  int c = 0;
  if (0 == n) {
    zero_padding = 1;
    prefix = 1;
  }
  char str[32];
  int i = 0;

  int is_signed = 0;

  if (n < 0) {
    is_signed = 1;
    n *= -1;
  }

  for (; n != 0 && i < 32; i++, n /= base)
    str[i] = char_set[(n % base)];

  if (is_signed) {
    str[i] = '-';
    i++;
  }

  char t = (zero_padding) ? '0' : ' ';
  int orig_i = i;

  if (!right_padding) {
    for (; prefix - orig_i > 0; prefix--)
      FILE_WRITE(f, &t, 1, &c);
  }

  for (i--; i >= 0; i--)
    FILE_WRITE(f, &(str[i]), 1, &c);

  if (right_padding) {
    for (; prefix - orig_i > 0; prefix--)
      FILE_WRITE(f, &t, 1, &c);
  }
  return c;
}

int fprint_int(FILE *f, int n, int prefix, int zero_padding,
               int right_padding) {
  return fprint_num(f, n, 10, "0123456789", prefix, zero_padding,
                    right_padding);
}

int fprint_hex(FILE *f, int n, int prefix, int zero_padding,
               int right_padding) {
  return fprint_num(f, n, 16, "0123456789abcdef", prefix, zero_padding,
                    right_padding);
}

int fprint_octal(FILE *f, int n, int prefix, int zero_padding,
                 int right_padding) {
  return fprint_num(f, n, 8, "012345678", prefix, zero_padding, right_padding);
}

int print_string(FILE *f, const char *s, int *rc, int prefix, int right_padding,
                 int precision) {
  int l = strlen(s);
  char t = ' ';
  int c = 0;
  if (!right_padding) {
    if (prefix)
      assert(-1 == precision); // FIXME: Is this correct?
    for (; prefix - l > 0; prefix--)
      FILE_WRITE(f, &t, 1, &c);
  }
  int bl = precision;
  for (; *s; s++, (*rc)++) {
    if (precision != -1) {
      if (0 == bl)
        break;
      bl--;
    }
    int r;
    FILE_WRITE(f, (const unsigned char *)s, 1, &r);
    assert(r != 0);
  }
  if (right_padding) {
    assert(-1 == precision); // FIXME: Is this correct?
    for (; prefix - l > 0; prefix--)
      FILE_WRITE(f, &t, 1, &c);
  }
  (*rc) += c;
  return 0;
}

int parse_precision(const char **fmt) {
  const char *s = *fmt;
  int rc = 0;
  for (int i = 0;; i++, s++) {
    if ('\0' == *s)
      break;
    const char c = *s;
    if ('*' == c) {
      assert(i == 0);
      return -1;
    } else if (!(c >= '0' && c <= '9')) {
      s--;
      break;
    }
    rc *= 10;
    rc += c - '0';
  }
  *fmt = s;
  return rc;
}

int vfprintf(FILE *f, const char *fmt, va_list ap) {
  int rc = 0;
  const char *s = fmt;
  int prefix = 0;

  int zero_padding = 0;
  int right_padding = 0;

  int cont = 0;
  int precision = -1;
  for (; *s; s++) {
    if (!cont && '%' != *s) {
      FILE_WRITE(f, (const unsigned char *)s, 1, &rc);
      continue;
    }
    if (!cont) {
      cont = 1;
      continue;
    }

    if ('\0' == *s)
      break;

    switch (*s) {
    case '.':
      s++;
      assert('\0' != *s);
      precision = parse_precision(&s);
      assert('\0' != *s);
      if (-1 == precision)
        precision = va_arg(ap, int);
      cont = 1;
      break;
    case '0':
      prefix *= 10;
      if (0 == prefix)
        zero_padding = 1;
      cont = 1;
      break;
    case '-':
      assert(0 == prefix);
      right_padding = 1;
      cont = 1;
      break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      prefix *= 10;
      prefix += (*s) - '0';
      cont = 1;
      break;
    case 'i':
    case 'd':
      if (-1 != precision) {
        zero_padding = 1;
        prefix = precision;
        right_padding = 0;
      }
      rc += fprint_int(f, va_arg(ap, int), prefix, zero_padding, right_padding);
      cont = 0;
      break;
    case 'u':
      assert(-1 == precision);
      rc += fprint_int(f, va_arg(ap, unsigned int), prefix, zero_padding,
                       right_padding);
      cont = 0;
      break;
    case 's': {
      assert(!zero_padding); // this is not supported to strings
      char *a = va_arg(ap, char *);
      if (!a) {
        if (-1 ==
            print_string(f, "(NULL)", &rc, prefix, right_padding, precision))
          return -1;
        cont = 0;
        break;
      }
      if (-1 == print_string(f, a, &rc, prefix, right_padding, precision))
        return -1;
      cont = 0;
      break;
    }
    case 'p': // TODO: Print this out in a nicer way
    case 'x':
      assert(-1 == precision);
      rc += fprint_hex(f, va_arg(ap, const uint32_t), prefix, zero_padding,
                       right_padding);
      cont = 0;
      break;
    case 'o':
      assert(-1 == precision);
      rc += fprint_octal(f, va_arg(ap, const uint32_t), prefix, zero_padding,
                         right_padding);
      cont = 0;
      break;
    case '%': {
      FILE_WRITE(f, (const unsigned char *)"%", 1, &rc);
      cont = 0;
      break;
    }
    case 'c': {
      char c = va_arg(ap, const int);
      FILE_WRITE(f, (const unsigned char *)&c, 1, &rc);
      cont = 0;
      break;
    }
    default:
      printf("got %c but that is not supported by printf\n", *s);
      assert(0);
      break;
    }
    if (!cont) {
      prefix = 0;
      zero_padding = right_padding = 0;
      precision = -1;
    }
  }
  fflush(f);
  return rc;
}
