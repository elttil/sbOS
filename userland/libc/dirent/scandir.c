#include <dirent.h>
#include <stdlib.h>
#include <string.h>

int nop_compar(const struct dirent **d1, const struct dirent **d2) {
  *d2 = *d1;
  return 0;
}

int scandir(const char *dir, struct dirent ***namelist,
            int (*sel)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **)) {
  if (!compar)
    compar = nop_compar;

  DIR *d = opendir(dir);
  if (!d)
    return -1;
  struct dirent **list = NULL;
  struct dirent *e;
  int rc = 0;
  for (; (e = readdir(d));) {
    if (sel) {
      if (!sel(e)) {
        continue;
      }
    }
    struct dirent *p = malloc(sizeof(struct dirent));
    memcpy(p, e, sizeof(struct dirent));
    list = realloc(list, (rc + 1) * sizeof(struct dirent *));
    list[rc] = p;
    rc++;
  }
  *namelist = list;
  closedir(d);
  return rc;
}
