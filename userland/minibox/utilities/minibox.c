#include "include.h"

void usage(void);
int main(int argc, char **argv);

// Should a command be called via "minibox <command> <arguments>" then shift
// argv and argc to become "<command> <arguments>" and rerun main()
int minibox_main(int argc, char **argv) {
  if (argc < 2)
    usage();

  argc--;
  argv++;
  return main(argc, argv);
}
