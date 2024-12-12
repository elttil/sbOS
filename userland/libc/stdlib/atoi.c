#include <stdlib.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/
int atoi(const char *str) {
  return (int)strtol(str, (char **)NULL, 10);
}
