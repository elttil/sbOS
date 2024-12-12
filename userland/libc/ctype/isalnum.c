#include <ctype.h>

// This is probably the most useless libc function I have seen so far.
int isalnum(int c) {
  return isalpha(c) || isdigit(c);
}
