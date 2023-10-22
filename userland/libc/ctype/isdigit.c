#include <ctype.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/isdigit.html
int isdigit(int c) {
  return ('0' <= c && c <= '9');
}
