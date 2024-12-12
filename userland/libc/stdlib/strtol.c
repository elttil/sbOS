#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

int get_value(char c, long base);

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/strtol.html
long strtol(const char *str, char **restrict endptr, int base) {
  long ret_value = 0;
  if (endptr) {
    *endptr = (char *)str;
  }
  // Ignore inital white-space sequence
  for (; *str && isspace(*str); str++)
    ;
  if (!*str) {
    return ret_value;
  }

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
      int val = get_value(*str, base);
      if (-1 == val) {
        break;
      }
      if (ret_value > LONG_MAX / base) {
        errno = ERANGE;
        return LONG_MAX;
      }
      ret_value *= base;
      if (ret_value > LONG_MAX - val) {
        errno = ERANGE;
        return LONG_MAX;
      }
      ret_value += val;
    }
  } else {
    errno = EINVAL;
    return 0;
  }
  if (endptr) {
    *endptr = (char *)str;
  }
  return ret_value;
}
