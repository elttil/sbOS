#ifndef UNISTD_H
#define UNISTD_H
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define STDIN_FILENO 0

extern int opterr, optind, optopt;
extern char *optarg;

int close(int fildes);
int ftruncate(int fildes, size_t length);
int execv(char *path, char **argv);
int pipe(int fd[2]);
int dup2(int org_fd, int new_fd);
int getopt(int argc, char *const argv[], const char *optstring);
pid_t getpid(void);
int unlink(const char *path);
int execvp(const char *file, char *const argv[]);
void _exit(int status);
void msleep(uint32_t ms); // not standard
uint32_t uptime(void);    // not standard
int chdir(const char *path);
char *getcwd(char *buf, size_t size);
int isatty(int fd);
#endif
