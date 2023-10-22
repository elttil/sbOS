#include <log.h>
#include <mmu.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

void kmalloc_allocate_heap(void);

void *kmalloc_eternal(size_t size);
void *kmalloc_eternal_align(size_t size);
void *kmalloc_eternal_physical(size_t size, void **physical);
void *kmalloc_eternal_physical_align(size_t size, void **physical);

void *kmalloc(size_t s);
void *kmalloc_align(size_t s);
void *krealloc(void *ptr, size_t size);
void *kreallocarray(void *ptr, size_t nmemb, size_t size);
void *kallocarray(size_t nmemb, size_t size);
void *kcalloc(size_t nelem, size_t elsize);
void kfree(void *p);
int init_heap(void);
