#include <stdio.h>
#include <stdint.h>

#define LOG_NOTE 3
#define LOG_WARN 2
#define LOG_ERROR 1
#define LOG_SUCCESS 0

void klog(char *str, int code);
void dump_backtrace(uint32_t max_frames);
