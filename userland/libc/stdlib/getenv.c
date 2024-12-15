#include <stdlib.h>
#include <string.h>

// Defined in stdlib/setenv.c
struct env {
  char *name;
  char *value;
  struct env *next;
};

struct env *internal_getenv(const char *name);

char *getenv(const char *name) {
  struct env *p = internal_getenv(name);
  if (!p) {
    return NULL;
  }
  return p->value;
}
