#include <stdio.h>

int vprintf(const char *format, va_list ap) {
  return vdprintf(1, format, ap);
}
