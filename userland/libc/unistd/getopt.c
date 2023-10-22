#include <assert.h>
#include <unistd.h>

int opterr, optind, optopt;
char *optarg;

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/getopt.html
int getopt(int argc, char *const argv[], const char *optstring) {
  // TODO
  optind = 1;
  optarg = NULL;
  //  assert(0);
  return -1;
}
