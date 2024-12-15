#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <tb/sb.h>
#include <tb/sv.h>
#include <unistd.h>

int execv(const char *pathname, char *const argv[]) {
  struct SYS_EXEC_PARAMS args = {.path = pathname, .argv = (char **)argv};
  RC_ERRNO(syscall(SYS_EXEC, &args, 0, 0, 0, 0));
}

int execvp(const char *file, char *const argv[]) {
  if ('/' == *file) {
    return execv(file, argv);
  }

  char *p = getenv("PATH");
  if (!p) {
    errno = ENOENT;
    return -1;
  }

  struct sv paths = C_TO_SV(p);

  struct sb builder;
  sb_init(&builder);
  for (;;) {
    struct sv path = sv_split_delim(paths, &paths, ':');
    if (0 == sv_length(path)) {
      break;
    }
    sb_reset(&builder);
    sb_append_sv(&builder, path);
    sb_append(&builder, "/");
    sb_append(&builder, file);
    sb_append_buffer(&builder, "\0", 1);

    if (-1 == execv(builder.string, argv)) {
      continue;
    }
  }
  sb_free(&builder);
  errno = ENOENT;
  return -1;
}
