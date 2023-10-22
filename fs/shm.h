#ifndef SHM_H
#define SHM_H
#include <stddef.h>
#include <stdint.h>

typedef int mode_t;

void shm_init(void);
int shm_open(const char *name, int oflag, mode_t mode);
int shm_unlink(const char *name);
int ftruncate(int fildes, uint64_t length);

#endif
