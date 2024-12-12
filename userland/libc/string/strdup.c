#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// The strdup() function shall return a pointer to a new string, which is a
// duplicate of the string pointed to by s. The returned pointer can be passed
// to free(). A null pointer is returned if the new string cannot be created.
char *strdup(const char *s) {
  size_t l = strlen(s);
  char *r = malloc(l + 1);
  if (!r) {
    return NULL;
  }
  strcpy(r, s);
  return r;
}
