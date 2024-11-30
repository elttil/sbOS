#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("provide command line arguments dumbass\n");
    return 1;
  }

  char *filename = argv[1];
  int fd = open(filename, O_READ, 0);
  if (-1 == fd) {
    perror("open");
    return 1;
  }
  int audio_fd = open("/dev/audio", O_WRITE, 0);
  if (-1 == audio_fd) {
    perror("open");
    return 1;
  }

  char *buffer = malloc(128000);

  for (;;) {
    int rc;
    if (0 == (rc = read(fd, buffer, 128000))) {
      break;
    }
    write(audio_fd, buffer, rc);
  }

  free(buffer);
  close(audio_fd);
  close(fd);
  return 0;
}
