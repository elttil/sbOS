#include <stdio.h>
#include <sys/types.h>

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
  if (!stream->has_control_over_the_fd) {
    if (stream->append) {
      fseek(stream, 0, SEEK_END);
    }
  }
  // FIXME: Check for overflow
  size_t bytes_to_write = nmemb * size;
  size_t rc = stream->write(stream, ptr, bytes_to_write);
  // On  success, fwrite() return the number of items
  // written.
  rc /= size;
  fflush(stream);
  return rc;
}
