#ifndef STDIO_H
#define STDIO_H
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// FIXME: Most of these should probably not be here. But I am too lazy
// to fix it right now. This is futures mees problem to deal wth.

#define EOF (-1)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct __IO_FILE FILE;
struct __IO_FILE {
  size_t (*write)(FILE *, const unsigned char *, size_t);
  size_t (*read)(FILE *, unsigned char *, size_t);
  int (*seek)(FILE *, long, int);
  long offset_in_file;
  int buffered_char;
  int has_buffered_char;
  uint8_t is_eof;
  uint8_t has_error;
  uint64_t file_size;
  void *cookie;
};

size_t write_fd(FILE *f, const unsigned char *s, size_t l);
size_t read_fd(FILE *f, unsigned char *s, size_t l);
int seek_fd(FILE *stream, long offset, int whence);

typedef struct {
  int fd;
} FILE_FD_COOKIE;

extern FILE __stdin_FILE;
extern FILE __stdout_FILE;
extern FILE __stderr_FILE;

#define stdin (&__stdin_FILE)
#define stdout (&__stdout_FILE)
//#define stderr (&__stderr_FILE)
#define stderr (&__stdout_FILE)

typedef int mode_t;

void perror(const char *s);

int putchar(int c);
int puts(const char *s);
int brk(void *addr);
void *sbrk(intptr_t increment);
int write(int fd, const char *buf, size_t count);
int pwrite(int fd, const char *buf, size_t count, size_t offset);
int printf(const char *format, ...);
int pread(int fd, void *buf, size_t count, size_t offset);
int read(int fd, void *buf, size_t count);
int fork(void);
int memcmp(const void *s1, const void *s2, size_t n);
int wait(int *stat_loc);
void exit(int status);
void *memcpy(void *dest, const void *src, uint32_t n);
int shm_open(const char *name, int oflag, mode_t mode);
int dprintf(int fd, const char *format, ...);
int vdprintf(int fd, const char *format, va_list ap);
int vprintf(const char *format, va_list ap);
int snprintf(char *str, size_t size, const char *format, ...);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int vfprintf(FILE *f, const char *fmt, va_list ap);
int fgetc(FILE *stream);
int getchar(void);
#define getc(_a) fgetc(_a)
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
FILE *fopen(const char *pathname, const char *mode);
int fclose(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
int fprintf(FILE *f, const char *fmt, ...);
long strtol(const char *nptr, char **endptr, int base);
char *strchr(const char *s, int c);
char *strcat(char *s1, const char *s2);
char *fgets(char *s, int n, FILE *stream);
FILE *tmpfile(void);
int feof(FILE *stream);
int fscanf(FILE *stream, const char *format, ...);
int ungetc(int c, FILE *stream);
long ftell(FILE *stream);
int fputc(int c, FILE *stream);
int remove(const char *path);
int ferror(FILE *stream);
int fputs(const char *s, FILE *stream);
int fflush(FILE *stream);
#endif
