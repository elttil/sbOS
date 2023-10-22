#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>

int main(int argc, char **argv) {
  DIR *d = opendir("/");
  struct dirent *de = readdir(d);
  printf("x : %x\n", de->d_name[0]);
  for (;;)
    ;
  /*
  int fd = 0;
  if (argc < 2)
    goto read_stdin;
  for (int i = 1; i < argc; i++) {
    if ((fd = open(argv[i], O_RDONLY, 0)) == -1) {
      return 1;
    }
  read_stdin:
    for (char c[4096], int rc; (rc = read(fd, c, 4096) > 0);)
      write(1, c, rc);
  }*/
  return 0;
}
