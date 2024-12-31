#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

int readdir_multi(DIR *dir, struct dirent *entries, int num_entries) {
  int rc;
  if (-1 ==
      (rc = read(dir->fd, entries, num_entries * sizeof(struct dirent)))) {
    return -1;
  }

  int num_read_entries = rc / sizeof(struct dirent);
  return num_read_entries;
}

struct dirent *readdir(DIR *dir) {
  int rc;
  if (-1 ==
      (rc = read(dir->fd, &dir->internal_direntry, sizeof(struct dirent)))) {
    return NULL;
  }
  if (rc < (int)sizeof(struct dirent)) {
    return NULL;
  }

  return &(dir->internal_direntry);
}
