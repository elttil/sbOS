#include <dirent.h>

int closedir(DIR *dir) {
  close(dir->fd);
  free(dir);
  return 0;
}
