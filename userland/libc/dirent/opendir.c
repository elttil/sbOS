#include <dirent.h>

DIR *fdopendir(int fd) {
  DIR *rc = malloc(sizeof(DIR));
  if (!rc) {
    return NULL;
  }
  rc->fd = fd;
  rc->dir_num = 0;
  return rc;
}

DIR *opendir(const char *dirname) {
  int fd = open(dirname, O_RDONLY, 0);
  if (-1 == fd) {
    return NULL;
  }
  DIR *rc = malloc(sizeof(DIR));
  if (!rc) {
    return NULL;
  }
  rc->fd = fd;
  rc->dir_num = 0;
  return rc;
}
