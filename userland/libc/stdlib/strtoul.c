#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

extern int errno;
int get_value(char c, long base) {
  int r;
  if (c >= '0' && c <= '9')
    r = c - '0';
  else if (c >= 'A' && c <= 'Z')
    r = c - 'A';
  else if (c >= 'a' && c <= 'z')
    r = c - 'a';
  else
    return -1;
  if (r >= base)
    return -1;
  return r;
}

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/strtoul.html
unsigned long strtoul(const char *restrict str, char **restrict endptr,
                      int base) {
  unsigned long ret_value = 0;
  if (endptr)
    *endptr = (char *)str;
  // Ignore inital white-space sequence
  for (; *str && isspace(*str); str++)
    ;
  if (!*str)
    return ret_value;

  //  int sign = 0;
  if ('-' == *str) {
    // FIXME
    //    sign = 1;
    str++;
    assert(0);
  } else if ('+' == *str) {
    str++;
  }

  if (0 == base) {
    char prefix = *str;
    if ('0' == prefix) {
      str++;
      if ('x' == tolower(*str)) {
        str++;
        base = 16;
      } else {
        base = 8;
      }
    } else {
      base = 10;
    }
  }

  if (2 <= base && 36 >= base) {
    for (; *str; str++) {
      ret_value *= base;
      int val = get_value(*str, base);
      /*
      if (-1 == val) {
        errno = ERANGE;
        return 0;
      }*/
      if (ret_value > ULONG_MAX - val) {
        errno = ERANGE;
        return 0;
      }

      ret_value += val;
    }
  } else {
    errno = EINVAL;
    return 0;
  }
  if (endptr)
    *endptr = (char*)str;
  return ret_value;
}
