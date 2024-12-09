#include "include.h"
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int lock_main(int argc, char **argv) {
  if (argc < 2) {
    fprintf("Usage: %s <password>", argv[0] ? argv[0] : "lock");
    return 1;
  }
  char *password = argv[1];
  size_t buffer_length = strlen(password) + 1;
  char *buffer = malloc(buffer_length);

  int should_exit = 0;
  size_t i = 0;
  for (;;) {
    char c;
    if (0 == read(STDIN_FILENO, &c, 1)) {
      break;
    }
    if ('\n' == c) {
      break;
    }
    if (i > buffer_length) {
      should_exit = 1;
      continue;
    }
    buffer[i] = c;
    i++;
  }
  buffer[i] = '\0';

  if (should_exit) {
    return 1;
  }

  if (0 != strcmp(password, buffer)) {
    return 1;
  }
  return 0;
}
