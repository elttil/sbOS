#include <ctype.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/isalpha.html
int isalpha(int c) {
  return (('A' <= toupper(c)) && ('Z' >= toupper(c)));
}
