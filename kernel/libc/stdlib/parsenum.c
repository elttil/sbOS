#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#define MAX_64 0xFFFFFFFFFFFFFFFF

extern int errno;
int get_value(char c, long base) {
  int r;
  int l = tolower(c);
  if (l >= '0' && l <= '9') {
    r = l - '0';
  } else if (l >= 'a' && l <= 'z') {
    r = (l - 'a') + 10;
  } else {
    return -1;
  }
  if (r >= base) {
    return -1;
  }
  return r;
}

u64 parse_u64(const char *restrict str, char **restrict endptr, int base,
              int *err) {
  if (err) {
    *err = 0;
  }
  u64 ret_value = 0;
  if (endptr) {
    *endptr = (char *)str;
  }
  if (!*str) {
    return ret_value;
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
      if (-1 == val) {
        break;
      }
      ret_value *= base;
      if (ret_value > MAX_64 - val) {
        if (err) {
          *err = 1;
        }
        return 0;
      }

      ret_value += val;
    }
  } else {
    if (err) {
      *err = 1;
    }
    return 0;
  }
  if (endptr) {
    *endptr = (char *)str;
  }
  return ret_value;
}
