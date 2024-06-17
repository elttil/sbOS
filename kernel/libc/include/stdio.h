#ifndef STDIO_H
#define STDIO_H
#include <stdarg.h>

void putc(const char c);
void delete_characther(void);
int kprintf(const char *format, ...);
int vkprintf(const char *format, va_list list);

#endif
