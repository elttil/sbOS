#ifndef MMAP_H
#define MMAP_H
#include <stddef.h>
#include <stdint.h>

#define MAP_FAILED ((void *)-1)

#define PROT_READ (1 << 0)
#define PROT_WRITE (1 << 1)
#define PROT_EXEC (1 << 2)

#define MAP_PRIVATE (1 << 0)
#define MAP_ANONYMOUS (1 << 1)
#define MAP_SHARED (1 << 2)

void *mmap(void *addr, size_t length, int prot, int flags, int fd,
           size_t offset);
int munmap(void *addr, size_t length);
#endif
