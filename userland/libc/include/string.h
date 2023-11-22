#ifndef STRING_H
#define STRING_H
#include <stddef.h>
#include <stdint.h>

char *strerror(int errnum);
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, uint32_t n);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
size_t strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);
int sscanf(const char *s, const char *restrict format, ...);
char *strrchr(const char *s, int c);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *s1, const char *s2, size_t n);
size_t *strlcpy(char *s1, const char *s2, size_t n);
size_t strcspn(const char *s1, const char *s2);
char *strpbrk(const char *s1, const char *s2);
size_t strspn(const char *s1, const char *s2);
void *memmove(void *s1, const void *s2, size_t n);
char *strdup(const char *s);
char *strndup(const char *s, size_t size);
char *strstr(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
int strcasecmp(const char *s1, const char *s2);
char *strtok(char *restrict s, const char *restrict sep);
char *strcat(char *restrict s1, const char *restrict s2);
int memcmp(const void *s1, const void *s2, size_t n);
#endif
