#ifndef UNISTD_H
#define UNISTD_H
#include <stddef.h>

extern int opterr, optind, optopt;
extern char *optarg;

int close(int fildes);
int ftruncate(int fildes, size_t length);
int execv(char *path, char **argv);
int pipe(int fd[2]);
int dup2(int org_fd, int new_fd);
int getopt(int argc, char * const argv[], const char *optstring);
#endif
