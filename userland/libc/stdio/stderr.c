#include <stdio.h>
#include <unistd.h>

FILE __stderr_FILE = {
    .write = write_fd,
    .read = read_fd,
    .is_eof = 0,
    .has_error = 0,
    .cookie = NULL,
    .fd = 2,
};
