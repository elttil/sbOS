#include <ctype.h>

int isprint(int c) {
  return c > 0x20 && 0x7F != c;
}
