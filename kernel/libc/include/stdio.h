#ifndef STDIO_H
#define STDIO_H
#include <stdarg.h>
#include <stddef.h>
#include <lib/sb.h>

void putc(const char c);
void delete_characther(void);
int kprintf(const char *format, ...);
int vkprintf(const char *format, va_list list);
int ksnprintf(char *out, size_t size, const char *format, ...);
int kbnprintf(char *out, size_t size, const char *format, ...);
int ksbprintf(struct sb *ctx, const char *format, ...);

#endif
