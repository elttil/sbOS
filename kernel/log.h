#include <stdio.h>
#include <typedefs.h>

#define LOG_NOTE 3
#define LOG_WARN 2
#define LOG_ERROR 1
#define LOG_SUCCESS 0

void log_char(const char c);
void klog(int code, char *fmt, ...);
void dump_backtrace(u32 max_frames);
void log_enable_screen(void);
