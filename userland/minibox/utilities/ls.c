#include "include.h"
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int ls_main(int argc, char **argv) {
  int list_hidden = 0;
  int newline = 0;
  /*
          for (int ch;-1 != (ch = getopt(argc, argv, "a1"));)
                  switch ((char)ch)
                  {
                  case 'a':
                          list_hidden = 1;
                          break;
                  case '1':
                          newline = 1;
                          break;
                  }*/
  char *path = argv[1];
  char path_buffer[256];

  if (!path) {
    (void)getcwd(path_buffer, 256);
    path = path_buffer;
  }

  struct dirent **namelist;
  int n;
  COND_PERROR_EXP(-1 == (n = scandir(path, &namelist, 0, 0)), "scandir",
                  return 1);

  int prev = 0;
  for (int i = 0; i < n; i++) {
    if (!list_hidden) {
      if ('.' == *namelist[i]->d_name) {
        continue;
      }
    }

    if (prev) {
      putchar(newline ? '\n' : ' ');
    } else {
      prev = 1;
    }
    printf("%s", namelist[i]->d_name);
  }
  putchar('\n');
  scandir_sane_free(namelist);
  return 0;
}
