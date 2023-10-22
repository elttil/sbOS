#include <stdio.h>
#include <sys/types.h>

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  // FIXME: Check for overflow
  ssize_t bytes_to_read = nmemb * size;
  size_t rc = stream->read(stream, ptr, bytes_to_read);
  // On  success, fread() return the number of items read
  rc /= size;
  return rc;
}
