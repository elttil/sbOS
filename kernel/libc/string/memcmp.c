#include "../include/string.h"

int memcmp(const void *s1, const void *s2, u32 n) {
  int return_value = 0;

  for (u32 i = 0; i < n; i++)
    if (((unsigned char *)(s1))[i] != ((unsigned char *)(s2))[i])
      return_value++;

  return return_value;
}
