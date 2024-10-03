#include <dirent.h>
#include <unistd.h>

int readdir_multi(DIR *dir, struct dirent *entries, int num_entries) {
  size_t offset = dir->dir_num * sizeof(struct dirent);
  int rc;
  if (-1 == (rc = pread(dir->fd, entries, num_entries * sizeof(struct dirent),
                        offset))) {
    return -1;
  }

  int num_read_entries = rc / sizeof(struct dirent);
  dir->dir_num += num_read_entries;
  return num_read_entries;
}

struct dirent *readdir(DIR *dir) {
  size_t offset = dir->dir_num * sizeof(struct dirent);
  int rc;
  if (-1 == (rc = pread(dir->fd, &dir->internal_direntry, sizeof(struct dirent),
                        offset)))
    return NULL;
  if (rc < (int)sizeof(struct dirent))
    return NULL;

  dir->dir_num++;
  return &(dir->internal_direntry);
}
