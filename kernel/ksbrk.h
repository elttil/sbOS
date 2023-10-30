#ifndef KSBRK_H
#define KSBRK_H
#include <stddef.h>
#include <stdint.h>

void* ksbrk(size_t s);
void *ksbrk_physical(size_t s, void **physical);
#endif
