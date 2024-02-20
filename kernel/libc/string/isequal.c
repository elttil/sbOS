#include "../include/string.h"

int isequal(const char *s1, const char *s2) {
  for (; *s1; s1++, s2++) {
    if (*s1 != *s2) {
      return 0;
    }
  }
  return 1;
}

int isequal_n(const char *s1, const char *s2, u32 n) {
  for (; *s1 && n; s1++, s2++, n--) {
    if (*s1 != *s2) {
      return 0;
    }
  }
  return 1;
}
