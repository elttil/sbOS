#ifndef STRING_H
#define STRING_H
#include <stddef.h>
#include <typedefs.h>

unsigned long strlen(const char *s);
void *memcpy(void *dest, const void *src, u32 n);
void *memset(void *dst, const unsigned char c, u32 n);
int memcmp(const void *s1, const void *s2, u32 n);
char *strcpy(char *d, const char *s);
int strcmp(const char *s1, const char *s2);
int isequal(const char *s1, const char *s2);
int isequal_n(const char *s1, const char *s2, u32 n);
char *copy_and_allocate_string(const char *s);
char *copy_and_allocate_user_string(const char *s);
size_t strlcpy(char *dst, const char *src, size_t dsize);
char *strcat(char *s1, const char *s2);
void *memmove(void *s1, const void *s2, size_t n);
#endif
