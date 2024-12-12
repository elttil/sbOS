#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <typedefs.h>

u32 sendfile(int out_fd, int in_fd, off_t *offset, size_t count, int *error_rc);
