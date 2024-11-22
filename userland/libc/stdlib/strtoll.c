#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

int get_value(char c, long base);

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/strtoll.html
long long strtoll(const char *str, char **restrict endptr, int base) {
  long long ret_value = 0;
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
      if (ret_value > LLONG_MAX / base) {
        errno = ERANGE;
        return LLONG_MAX;
      }
      ret_value *= base;
      int val = get_value(*str, base);
      if (ret_value > LLONG_MAX - val) {
        errno = ERANGE;
        return LLONG_MAX;
      }
      ret_value += val;
    }
  } else {
    errno = EINVAL;
    return 0;
  }
  if (endptr)
    *endptr = (char *)str;
  return ret_value;
}
