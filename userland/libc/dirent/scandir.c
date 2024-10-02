#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int nop_compar(const struct dirent **d1, const struct dirent **d2) {
  *d2 = *d1;
  return 0;
}

// Differs from scandir in the sense that the namelist has its own way
// of allocating its entries(in this case it allocates it as a large
// chunk). As a result the usual free() operation can not be ran on
// the elements.
//
// Instead scandir_sane_free() should be used to deallocate the
// namelist.
int scandir_sane(const char *dir, struct dirent ***namelist,
                 int (*sel)(const struct dirent *),
                 int (*compar)(const struct dirent **,
                               const struct dirent **)) {
  if (!compar) {
    compar = nop_compar;
  }

  int fd = open(dir, O_RDONLY);
  if (-1 == fd) {
    // TODO: Figure out what to set errno to
    return -1;
  }

  size_t capacity = 128;
  size_t num_entries;
  struct dirent *chunk = NULL;
  for (;;) {
    chunk = realloc(chunk, sizeof(struct dirent) * capacity);
    int rc = pread(fd, chunk, sizeof(struct dirent) * capacity, 0);
    if (-1 == rc) {
      free(chunk);
      // TODO: Figure out what to set errno to
      return -1;
    }
    if (sizeof(struct dirent) * capacity == (size_t)rc) {
      // The directory **may** contain more entries.
      // Redo
      capacity *= 2;
      continue;
    }
    num_entries = rc / (sizeof(struct dirent));
    break;
  }

  *namelist = allocarray(num_entries, sizeof(struct dirent *));

  for (size_t i = 0; i < num_entries; i++) {
    (*namelist)[i] = &chunk[i];
  }

  close(fd);
  return num_entries;
}

void scandir_sane_free(struct dirent **namelist) {
  free(namelist[0]); // First pointer is the start of the chunk in the
                     // current implementation
  free(namelist);
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
