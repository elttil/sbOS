#include <ctype.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/isxdigit.html
int isxdigit(int c) {
  return isdigit(c) || ('A' >= toupper(c) && 'Z' <= toupper(c));
}
