// cat - concatenate and print files
// https://pubs.opengroup.org/onlinepubs/9699919799/
//
// TODO: Add -u flag "Write bytes from the input file to the standard
// output without delay as each is read."
#include "include.h"
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int fd_to_stdout(int fd) {
  int rc;
  for (char buffer[CAT_BUFFER]; (rc = read(fd, &buffer, sizeof(buffer)));) {
    if (-1 == rc)
      return 0;
    if (-1 == write(fd_stdout, buffer, rc))
      return 0;
  }
  return 1;
}

int cat_main(int argc, char **argv) {
  int fd = STDIN_FILENO;

  // If no file operands are specified, the standard input shall be
  // used.
  if (argc < 2) {
    return (fd_to_stdout(0)) ? 0 : 1;
  }

  argv++;
  for (; *argv; argv++) {
    // If a file is '-', the cat utility shall read from the standard
    // input at that point in the sequence.
    if (0 == strcmp(*argv, "-")) {
      if (!fd_to_stdout(0))
        return 1;
      continue;
    }

    if (-1 == (fd = open(*argv, O_RDONLY, 0))) {
      printf("cat: ");
      //      fflush(stdout);
      perror(*argv);
      return 1;
    }
    if (!fd_to_stdout(fd))
      return 1;
    close(fd);
  }
  return 0;
}
