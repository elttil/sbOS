#ifndef SHM_H
#define SHM_H
#include <stddef.h>
#include <typedefs.h>

typedef int mode_t;

void shm_init(void);
int shm_open(const char *name, int oflag, mode_t mode);
int shm_unlink(const char *name);
int ftruncate(int fildes, u64 length);

#endif
