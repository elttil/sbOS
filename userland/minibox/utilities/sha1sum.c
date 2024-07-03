#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <tb/sha1.h>
#include <unistd.h>

static int sha1_hash_file(const char *name, int fd) {
  SHA1_CTX ctx;
  SHA1_Init(&ctx);

  for (;;) {
    char buffer[4096];
    int rc = read(fd, buffer, 4096);
    if (-1 == rc) {
      perror("read");
      return 0;
    }
    if (0 == rc) {
      break;
    }
    SHA1_Update(&ctx, buffer, rc);
  }
  unsigned char digest[SHA1_LEN];
  SHA1_Final(&ctx, digest);

  printf("%s: ", name);
  for (int i = 0; i < SHA1_LEN; i++) {
    printf("%02x", digest[i]);
  }
  printf("\n");
  return 1;
}

int sha1sum_main(int argc, char **argv) {
  int fd = STDIN_FILENO;

  // If no file operands are specified, the standard input shall be
  // used.
  if (argc < 2) {
    return (sha1_hash_file("-", 0)) ? 0 : 1;
  }

  argv++;
  for (; *argv; argv++) {
    if (0 == strcmp(*argv, "-")) {
      if (!sha1_hash_file("-", 0)) {
        return 1;
      }
      continue;
    }

    if (-1 == (fd = open(*argv, O_RDONLY, 0))) {
      perror(*argv);
      return 1;
    }
    if (!sha1_hash_file(*argv, fd)) {
      return 1;
    }
    close(fd);
  }
  return 0;
}
