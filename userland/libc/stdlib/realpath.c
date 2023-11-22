#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// FIXME: This is nowhere near complete
char *realpath(const char *filename, char *resolvedname) {
  assert(resolvedname);
  // FIXME: This really should have bounds checking
  char cwd[256];
  getcwd(cwd, 256);
  strcat(cwd, filename);
  const char *path = cwd;
  char *result = resolvedname;
  // It has to be a absolute path
  if ('/' != *path)
    return 0;
  const char *result_start = result;
  int start_directory = 0;
  int should_insert_slash = 0;
  for (; *path; path++) {
    if (start_directory) {
      start_directory = 0;
      if ('/' == *path) {
        path++;
      } else if (0 == memcmp(path, "./", 2) || 0 == memcmp(path, ".\0", 2)) {
        path++;
      } else if (0 == memcmp(path, "../", 3) || 0 == memcmp(path, "..\0", 3)) {
        path += 2;
        if (result_start + 2 > result) {
          // The path is probably something like "/.." or
          // "/../foo". A "/.." should become a "/"
          // Therefore it skips going back to the parent
          if (*path == '/') {
            if (result_start == result)
              return 0;
            result--;
          }
        } else {
          if ('/' != *path) {
            should_insert_slash = 1;
          }
          result--;
          result--;
          for (; result_start <= result && '/' != *result; result--)
            ;
        }
      }
    }
    start_directory = ('/' == *path);
    if ('\0' == *path)
      break;
    *result = *path;
    result++;
  }
  if (should_insert_slash) {
    *result = '/';
    result++;
  }
  *result = '\0';
  return 1;
}
