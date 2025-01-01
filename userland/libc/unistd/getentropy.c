#include <errno.h>
#include <stdio.h>
#include <sys/random.h>

int getentropy(void *buffer, size_t length) {
  if (length > 256) {
    errno = EIO;
    return -1;
  }
  randomfill(buffer, length);
  return 0;
}
