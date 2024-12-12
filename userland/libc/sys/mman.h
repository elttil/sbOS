#ifndef MMAP_H
#define MMAP_H
#include <stddef.h>
#include <stdint.h>

void *mmap(void *addr, size_t length, int prot, int flags, int fd,
           size_t offset);
#endif
