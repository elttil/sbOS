#ifndef STDLIB_H
#define STDLIB_H
#include <stddef.h>
#include <limits.h>
#define RAND_MAX (UINT32_MAX)

void *malloc(size_t s);
void *calloc(size_t nelem, size_t elsize);
void *realloc(void *ptr, size_t size);
void free(void *p);
char *getenv(const char *name);
int rand(void);
void srand(unsigned int seed);
unsigned long strtoul(const char *restrict str,
       char **restrict endptr, int base);
int atoi(const char *str);
#endif
