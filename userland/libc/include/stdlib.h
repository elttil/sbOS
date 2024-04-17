#ifndef STDLIB_H
#define STDLIB_H
#include <limits.h>
#include <stddef.h>
#define RAND_MAX (UINT32_MAX)
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

typedef size_t size_t; // only for 32 bit

void *malloc(size_t s);
void *calloc(size_t nelem, size_t elsize);
void *realloc(void *ptr, size_t size);
void free(void *p);
char *getenv(const char *name);
int rand(void);
void srand(unsigned int seed);
unsigned long strtoul(const char *restrict str, char **restrict endptr,
                      int base);
long strtol(const char *str, char **restrict endptr, int base);
long long strtoll(const char *str, char **restrict endptr, int base);
void abort(void);
int abs(int i);
void qsort(void *base, size_t nel, size_t width,
           int (*compar)(const void *, const void *));
int atexit(void (*func)(void));
int mkstemp(char *template);
long double strtold(const char *restrict nptr, char **restrict endptr);
int system(const char *command);
double atof(const char *str);
double strtod(const char *restrict nptr, char **restrict endptr);
int atoi(const char *str);
char *realpath(const char *filename, char *resolvedname);
long atol(const char *nptr);
long long atoll(const char *nptr);
#endif
