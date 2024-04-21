#ifndef DIRENT_H
#define DIRENT_H
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#ifndef KERNEL
#include <unistd.h>
#endif

struct dirent {
  ino_t d_ino;           // File serial number.
  char d_name[PATH_MAX]; // Filename string of entry.
};

typedef struct {
  int fd;
  struct dirent internal_direntry;
  int dir_num;
} DIR;

DIR *opendir(const char *dirname);
struct dirent *readdir(DIR *dir);
int closedir(DIR *dirp);
int alphasort(const struct dirent **d1, const struct dirent **d2);
int scandir(const char *dir, struct dirent ***namelist,
            int (*sel)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **));
#endif
