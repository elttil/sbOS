#include <stdint.h>
#include <typedefs.h>
#include <stddef.h>
#include <sys/types.h>

u32 sendfile(int out_fd, int in_fd, off_t *offset, size_t count,
             int *error_rc);
