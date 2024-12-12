#include <string.h>

int memcmp(const void *s1, const void *s2, size_t n) {
  int return_value = 0;

  for (uint32_t i = 0; i < n; i++) {
    if (((unsigned char *)(s1))[i] != ((unsigned char *)(s2))[i]) {
      return_value++;
    }
  }

  return return_value;
}
