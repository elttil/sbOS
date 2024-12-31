#include <tb/sv.h>

char* handle_create(struct sv path, int is_dir, int flags, int mode);
int handle_get_fd(struct sv key, int *fd, char **pathname);
int handle_close(struct sv key);
