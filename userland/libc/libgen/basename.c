#include <libgen.h>
#include <stddef.h>

/*
The basename() function shall take the pathname pointed to by path and
return a pointer to the final component of the pathname, deleting any
trailing '/' characters.


The basename() function may modify the string pointed to by path, and
may return a pointer to internal storage. The returned pointer might be
invalidated or the storage might be overwritten by a subsequent call to
basename(). The returned pointer might also be invalidated if the
calling thread is terminated.
*/

char *basename_empty_return_value = ".";
char *basename_slash_return_value = "/";
char *basename(char *path) {
  // If path is a null pointer or points to an empty string, basename()
  // shall return a pointer to the string ".".
  if (NULL == path || '\0' == *path)
    return basename_empty_return_value;

  char *start = path;
  // Move the string to the end
  for (; *path; path++)
    ;
  if (start == path)
    return start;
  path--;
  if ('/' == *path) // Trailing slash
    *path = '\0';
  // Loop until next slash is found
  for (; path != start && '/' != *path; path--)
    ;
  // If the string pointed to by path consists entirely of the '/' character,
  // basename() shall return a pointer to the string "/". If the string
  // pointed to by path is exactly "//", it is implementation-defined whether
  //'/' or "//" is returned.
  path++;
  if ('\0' == *path)
    return basename_slash_return_value;
  return path;
}
