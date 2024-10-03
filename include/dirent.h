#ifndef DIRENT_H
#define DIRENT_H
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#ifndef KERNEL
#include <fcntl.h>
#include <unistd.h>
#endif // KERNEL

struct dirent {
  ino_t d_ino;           // File serial number.
  char d_name[PATH_MAX]; // Filename string of entry.
};

typedef struct {
  int fd;
  struct dirent internal_direntry;
  int dir_num;
} DIR;

#ifndef KERNEL
DIR *opendir(const char *dirname);
DIR *fdopendir(int fd);
struct dirent *readdir(DIR *dir);
int closedir(DIR *dirp);
int alphasort(const struct dirent **d1, const struct dirent **d2);
int scandir(const char *dir, struct dirent ***namelist,
            int (*sel)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **));
int scandir_sane(const char *dir, struct dirent ***namelist,
                 int (*sel)(const struct dirent *),
                 int (*compar)(const struct dirent **,
                               const struct dirent **));
void scandir_sane_free(struct dirent **namelist);
int readdir_multi(DIR *dir, struct dirent *entries, int num_entries);
#endif // KERNEL
#endif // DIRENT_H
