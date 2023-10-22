#include <dirent.h>

DIR *opendir(const char *dirname) {
  int fd = open(dirname, O_RDONLY, 0);
  if (-1 == fd)
    return NULL;
  DIR *rc = malloc(sizeof(DIR));
  rc->fd = fd;
  rc->dir_num = 0;
  return rc;
}
