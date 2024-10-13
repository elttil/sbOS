#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <tb/sb.h>
#include <tb/sv.h>
#include <unistd.h>

char *realpath(const char *filename, char *resolvedname) {
  if (!filename) {
    return -EINVAL;
  }

  char cwd[256];
  getcwd(cwd, 256);
  strcat(cwd, filename); // FIXME: bounds check

  struct sb string;
  sb_init_capacity(&string, 256);

  struct sv path = C_TO_SV(cwd);

  int ignore = 0;
  int last_was_dotdot = 0;

  int was_root = 0;
  for (;;) {
    ignore = 0;
    was_root = 0;
    struct sv dir = sv_end_split_delim(path, &path, '/');

    if (sv_partial_eq(dir, C_TO_SV("/"))) {
      was_root = 1;
    }

    if (sv_eq(dir, C_TO_SV("/"))) {
      ignore = 1;
    }

    if (sv_eq(dir, C_TO_SV("/."))) {
      ignore = 1;
    }

    if (sv_eq(dir, C_TO_SV("/.."))) {
      last_was_dotdot = 1;
      ignore = 1;
    } else {
      if (last_was_dotdot) {
        ignore = 1;
      }
      last_was_dotdot = 0;
    }

    if (!ignore && !last_was_dotdot) {
      sb_prepend_sv(&string, dir);
    }

    if (sv_isempty(path)) {
      if (was_root && ignore && sb_isempty(&string)) {
        sb_prepend_sv(&string, C_TO_SV("/"));
      }
      break;
    }
  }
  char *result = sv_copy_to_c(SB_TO_SV(string), resolvedname, 256);
  sb_free(&string);
  return result;
}
