#ifndef MALLOC_H
#define MALLOC_H
#include <stddef.h>
#include <stdint.h>

void *malloc(size_t s);
void *calloc(size_t nelem, size_t elsize);
void *realloc(void *ptr, size_t size);
void free(void *p);
#endif
