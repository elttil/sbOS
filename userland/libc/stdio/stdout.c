#include <stdio.h>
#include <unistd.h>

FILE __stdout_FILE = {
    .write = write_fd,
    .read = read_fd,
    .is_eof = 0,
    .has_error = 0,
    .seek = NULL,
    .cookie = NULL,
    .fd = 1,
};
FILE __stderr_FILE;
