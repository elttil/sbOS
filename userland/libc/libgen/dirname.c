#include <libgen.h>

char *dirname_empty_return_value = ".";
char *dirname_slash_return_value = "/";
char *dirname(char *path) {
  // If path is a null pointer or points to an empty string,
  // dirname() shall return a pointer to the string "."
  if (!path) {
    return dirname_empty_return_value;
  }
  if ('\0' == *path) {
    return dirname_empty_return_value;
  }

  char *start = path;
  for (; *path; path++)
    ;
  path--;
  if ('/' == *path) {
    if (start == path) {
      return path;
    }
    path--;
  }

  for (; path != start && '/' != *path; path--)
    ;
  // If path does not contain a '/', then dirname() shall return a pointer to
  // the string ".".
  if ('/' != *path) {
    return dirname_empty_return_value;
  }

  if (path == start) {
    return dirname_slash_return_value;
  }

  *path = '\0';
  path--;

  for (; path != start && '/' != *path; path--)
    ;

  if ('/' != *path) {
    return dirname_empty_return_value;
  }

  path++;
  return path;
}
