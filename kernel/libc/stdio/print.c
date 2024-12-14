#include <assert.h>
#include <kmalloc.h>
#include <lib/sb.h>
#include <log.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define TAB_SIZE 8

struct print_context {
  void *data;
  void (*write)(struct print_context *, const char *, int);
};

#define WRITE(s, l, r)                                                         \
  {                                                                            \
    ctx->write(ctx, s, l);                                                     \
    *(int *)(r) += l;                                                          \
  }

int print_num(struct print_context *ctx, long long n, int base, char *char_set,
              int prefix, int zero_padding, int right_padding) {
  int c = 0;
  char str[32];
  int i = 0;

  int is_signed = 0;

  if (n < 0) {
    is_signed = 1;
    n *= -1;
  }

  if (0 == n) {
    str[i] = char_set[0];
    i++;
  } else {
    for (; n != 0 && i < 32; i++, n /= base) {
      str[i] = char_set[(n % base)];
    }
  }

  if (is_signed) {
    str[i] = '-';
    i++;
  }

  char t = (zero_padding) ? '0' : ' ';
  int orig_i = i;

  if (!right_padding) {
    for (; prefix - orig_i > 0; prefix--) {
      WRITE(&t, 1, &c);
    }
  }

  for (i--; i >= 0; i--) {
    WRITE(&(str[i]), 1, &c);
  }

  if (right_padding) {
    for (; prefix - orig_i > 0; prefix--) {
      WRITE(&t, 1, &c);
    }
  }
  return c;
}

int print_int(struct print_context *ctx, long long n, int prefix,
              int zero_padding, int right_padding) {
  return print_num(ctx, n, 10, "0123456789", prefix, zero_padding,
                   right_padding);
}

int print_hex(struct print_context *ctx, long long n, int prefix,
              int zero_padding, int right_padding) {
  return print_num(ctx, n, 16, "0123456789abcdef", prefix, zero_padding,
                   right_padding);
}

int print_octal(struct print_context *ctx, long long n, int prefix,
                int zero_padding, int right_padding) {
  return print_num(ctx, n, 8, "012345678", prefix, zero_padding, right_padding);
}

int print_string(struct print_context *ctx, const char *s, int *rc, int prefix,
                 int right_padding, int precision) {
  int l = strlen(s);
  char t = ' ';
  int c = 0;
  if (!right_padding) {
    if (prefix) {
      assert(-1 == precision); // FIXME: Is this correct?
    }
    for (; prefix - l > 0; prefix--) {
      WRITE(&t, 1, &c);
    }
  }
  int bl = precision;
  for (; *s; s++, (*rc)++) {
    if (precision != -1) {
      if (0 == bl) {
        break;
      }
      bl--;
    }
    int r = 0;
    WRITE((const unsigned char *)s, 1, &r);
  }
  if (right_padding) {
    assert(-1 == precision); // FIXME: Is this correct?
    for (; prefix - l > 0; prefix--) {
      WRITE(&t, 1, &c);
    }
  }
  (*rc) += c;
  return 0;
}

int parse_precision(const char **fmt) {
  const char *s = *fmt;
  int rc = 0;
  for (int i = 0;; i++, s++) {
    if ('\0' == *s) {
      break;
    }
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

int vkcprintf(struct print_context *ctx, const char *fmt, va_list ap) {
  int rc = 0;
  const char *s = fmt;
  int prefix = 0;

  int long_level = 0;

  int zero_padding = 0;
  int right_padding = 0;

  int cont = 0;
  int precision = -1;
  for (; *s; s++) {
    if (!cont && '%' != *s) {
      WRITE((const unsigned char *)s, 1, &rc);
      continue;
    }
    if (!cont) {
      cont = 1;
      continue;
    }

    if ('\0' == *s) {
      break;
    }

    switch (*s) {
    case '.':
      s++;
      assert('\0' != *s);
      precision = parse_precision(&s);
      assert('\0' != *s);
      if (-1 == precision) {
        precision = va_arg(ap, int);
      }
      cont = 1;
      break;
    case '0':
      prefix *= 10;
      if (0 == prefix) {
        zero_padding = 1;
      }
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
    case 'l':
      long_level++;
      assert(long_level <= 2);
      break;
    case 'i':
    case 'd':
      if (-1 != precision) {
        zero_padding = 1;
        prefix = precision;
        right_padding = 0;
      }
      if (2 == long_level) {
        rc += print_int(ctx, va_arg(ap, long long), prefix, zero_padding,
                        right_padding);
      } else if (1 == long_level) {
        rc += print_int(ctx, va_arg(ap, long long), prefix, zero_padding,
                        right_padding);
      } else {
        rc += print_int(ctx, va_arg(ap, int), prefix, zero_padding,
                        right_padding);
      }
      long_level = 0;
      cont = 0;
      break;
    case 'u':
      assert(-1 == precision);
      rc += print_int(ctx, va_arg(ap, unsigned int), prefix, zero_padding,
                      right_padding);
      cont = 0;
      break;
    case 's': {
      assert(!zero_padding); // this is not supported to strings
      char *a = va_arg(ap, char *);
      if (!a) {
        if (-1 == print_string(ctx, "(NULL)", &rc, prefix, right_padding,
                               precision)) {
          return -1;
        }
        cont = 0;
        break;
      }
      if (-1 == print_string(ctx, a, &rc, prefix, right_padding, precision)) {
        return -1;
      }
      cont = 0;
      break;
    }
    case 'p': // TODO: Print this out in a nicer way
    case 'x':
      assert(-1 == precision);
      rc += print_hex(ctx, va_arg(ap, const u32), prefix, zero_padding,
                      right_padding);
      cont = 0;
      break;
    case 'o':
      assert(-1 == precision);
      rc += print_octal(ctx, va_arg(ap, const u32), prefix, zero_padding,
                        right_padding);
      cont = 0;
      break;
    case '%': {
      WRITE((const unsigned char *)"%", 1, &rc);
      cont = 0;
      break;
    }
    case 'c': {
      char c = va_arg(ap, const int);
      WRITE((const unsigned char *)&c, 1, &rc);
      cont = 0;
      break;
    }
    default:
      kprintf("got %c but that is not supported by printf\n", *s);
      assert(0);
      break;
    }
    if (!cont) {
      prefix = 0;
      zero_padding = right_padding = 0;
      precision = -1;
    }
  }
  return rc;
}

void sb_write(struct print_context *_ctx, const char *s, int l) {
  struct sb *ctx = (struct sb *)_ctx->data;
  assert(ctx);
  sb_append_buffer(ctx, s, l);
}

int vksbprintf(struct sb *ctx, const char *format, va_list ap) {
  struct print_context context;

  context.data = ctx;
  context.write = sb_write;

  return vkcprintf(&context, format, ap);
}

int ksbprintf(struct sb *ctx, const char *format, ...) {
  va_list list;
  va_start(list, format);
  int rc = vksbprintf(ctx, format, list);
  va_end(list);
  return rc;
}

int kbnprintf(char *out, size_t size, const char *format, ...) {
  struct sb ctx;
  sb_init_buffer(&ctx, out, size);

  va_list list;
  va_start(list, format);
  int rc = vksbprintf(&ctx, format, list);
  va_end(list);
  return rc;
}

struct sn_context {
  char *out;
  int size;
};

void sn_write(struct print_context *_ctx, const char *s, int l) {
  struct sn_context *ctx = (struct sn_context *)_ctx->data;
  assert(ctx);
  size_t k = min(l, ctx->size);
  memcpy(ctx->out, s, k);
  ctx->out += k;
  ctx->size -= k;
  *(ctx->out) = '\0';
}

int ksnprintf(char *out, size_t size, const char *format, ...) {
  struct print_context context;

  struct sn_context ctx;
  context.data = &ctx;
  ctx.out = out;
  ctx.size = size;
  context.write = sn_write;

  va_list list;
  va_start(list, format);
  int rc = vkcprintf(&context, format, list);
  va_end(list);
  return rc;
}

void context_serial_write(struct print_context *ctx, const char *s, int l) {
  (void)ctx;
  for (; l > 0; l--, s++) {
    log_char(*s);
  }
}

struct print_context serial_context = {.data = NULL,
                                       .write = context_serial_write};

int vkprintf(const char *format, va_list ap) {
  return vkcprintf(&serial_context, format, ap);
}

int kprintf(const char *format, ...) {
  va_list list;
  va_start(list, format);
  int rc = vkprintf(format, list);
  va_end(list);
  return rc;
}
