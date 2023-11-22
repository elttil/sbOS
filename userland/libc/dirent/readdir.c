#include <dirent.h>
#include <unistd.h>

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
