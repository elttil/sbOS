#ifndef KMALLOC_H
#define KMALLOC_H
#include <stddef.h>
#include <typedefs.h>

void *kmalloc_align(size_t s, void **physical);
void kmalloc_align_free(void *p, size_t s);

void kmalloc_allocate_heap(void);

void kmalloc_scan(void);

void *kmalloc(size_t s);
void *krealloc(void *ptr, size_t size);
void *kreallocarray(void *ptr, size_t nmemb, size_t size);
void *kallocarray(size_t nmemb, size_t size);
void *kcalloc(size_t nelem, size_t elsize);
void *krecalloc(void *ptr, size_t nelem, size_t elsize);
void kfree(void *p);
int init_heap(void);
#endif
