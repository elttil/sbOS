// SPDX-License-Identifier: 0BSD
// TODO: Possibly use getprogname() instead of using argv[0] to get the
// utility name.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utilities/include.h"

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

typedef struct Command {
  char *name;
  int (*function)(int, char **);
} Command;

#define STR2(_x) #_x
#define STR(_x) STR2(_x)
#define COMMAND(NAME) {STR(NAME), NAME##_main}

Command utilities[] = {COMMAND(minibox), COMMAND(ascii), COMMAND(echo),
                       COMMAND(cat),     COMMAND(yes),   COMMAND(wc),
                       COMMAND(init),    COMMAND(ls),    COMMAND(touch),
                       COMMAND(ed),      COMMAND(sh),    COMMAND(kill),
                       COMMAND(sha1sum), COMMAND(rdate), COMMAND(true),
                       COMMAND(false),   COMMAND(lock)};

char *parse_filename(char *str) {
  char *tmp = NULL, *is = str;
  for (; *is++;) {
    if ('/' == *is) {
      tmp = is;
    }
  }
  return tmp ? tmp + 1 : str;
}

void usage(void) {
  for (int i = 0; i < ARRAY_LENGTH(utilities); i++) {
    printf("%s ", utilities[i].name);
  }
  printf("\n");
}

int main(int argc, char **argv) {
  if (argc < 1) {
    return 1;
  }
#ifdef SINGLE_MAIN
  return utilities[0].function(argc, argv);
#endif

  // argv[0] will be checked to determine what utility
  // is supposed to be ran.
  // NOTE: "minibox" is a utility than can be ran. "minibox"
  //       will switch argv and argc so that argv[1] -> argv[0]
  //       then it will rerun main(). This allows utlities
  //       to be ran like "minibox <utility> <arguments>" or
  //       even "minibox minibox <utility> <arguments>"
  const char *utility_name = parse_filename(argv[0]);
  if (*utility_name == '/') {
    utility_name++;
  }
  for (int i = 0; i < ARRAY_LENGTH(utilities); i++) {
    if (0 == strcmp(utility_name, utilities[i].name)) {
      return utilities[i].function(argc, argv);
    }
  }

  usage();
  return 0;
}
