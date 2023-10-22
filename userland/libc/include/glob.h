#ifndef GLOB_H
#define GLOB_H
#include <stddef.h>

typedef struct {
  size_t gl_pathc; // Count of paths matched by pattern.
  char **gl_pathv; // Pointer to a list of matched pathnames
  size_t gl_offs;  // Slots to reserve at the beginning of gl_pathv.
} glob_t;
#endif
