#include <string.h>
#include <unistd.h>

int opterr;
int optind = 1;
int optopt;
char *optarg;

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/getopt.html
int getopt(int argc, char *const *argv, const char *optstring) {
  // If, when getopt() is called:
  // argv[optind] is a null pointer
  if (optind > argc || !argv[optind]) {
    return -1;
  }
  // *argv[optind] is not the character -
  if ('-' != *argv[optind]) {
    return -1;
  }
  char option = *(argv[optind] + 1);
  // argv[optind] points to the string "-"
  if ('\0' == option) {
    return -1;
  }
  // getopt() shall return -1 without changing optind. If:
  // TODO: Should the other -1 returns update optind? That does not seem
  // to make sense(but the spec says that as a edge case).
  if (0 == strcmp(argv[optind], "--")) {
    return -1;
  }
  const char *p = optstring;
  for (; *p; p++) {
    if (option == *p) {
      break;
    }
  }
  optopt = option;
  if ('\0' == *p) {
    return '?';
  }
  optind++;
  if (':' == *(p + 1)) {
    optarg = argv[optind];
    optind++;
  }
  return optopt;
}
