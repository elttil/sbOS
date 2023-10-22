#ifndef STRING_H
#define STRING_H
#include <stddef.h>
#include <stdint.h>

unsigned long strlen(const char *s);
void *memcpy(void *dest, const void *src, uint32_t n);
void *memset(void *dst, const unsigned char c, uint32_t n);
int memcmp(const void *s1, const void *s2, uint32_t n);
char *strcpy(char *d, const char *s);
int strcmp(const char *s1, const char *s2);
int isequal(const char *s1, const char *s2);
int isequal_n(const char *s1, const char *s2, uint32_t n);
char *copy_and_allocate_string(const char *s);
char *copy_and_allocate_user_string(const char *s);
char *strncpy(char *dest, const char *src, size_t n);
size_t strlcpy(char *dst, const char *src, size_t dsize);
char *strcat(char *s1, const char *s2);
#endif
