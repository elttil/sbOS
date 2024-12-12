#include <stdlib.h>
#include <string.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/qsort.html
void qsort(void *base, size_t nel, size_t width,
           int (*compar)(const void *, const void *)) {
  // If the nel argument has the value zero, the comparison function pointed to
  // by compar shall not be called and no rearrangement shall take place.
  if (0 == nel) {
    return;
  }

  // AB
  // Results in negative
  // BA
  // Results in positive

  // Using bubblesort
  unsigned char *p = base;
  for (size_t i = 1; i < nel; i++) {
    for (size_t j = 0; j < nel; j++) {
      if (compar((p + i * width), (p + j * width)) < 0) {
        unsigned char tmp[width];
        memcpy(tmp, (p + i * width), width);
        memcpy((p + i * width), (p + j * width), width);
        memcpy((p + j * width), tmp, width);
      }
    }
  }
}
