#include <ctype.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/isascii.html
int isascii(int c) {
  return (c < 128);
}
