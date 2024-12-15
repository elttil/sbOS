#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct env {
  char *name;
  char *value;
  struct env *next;
};

struct env *env_head = NULL;

struct env *internal_getenv(const char *name) {
  struct env *p = env_head;
  for (; p; p = p->next) {
    if (0 == strcmp(p->name, name)) {
      return p;
    }
  }
  return NULL;
}

int setenv(const char *name, const char *value, int overwrite) {
  if (NULL == name) {
    errno = -EINVAL;
    return -1;
  }

  int name_length = strlen(name);
  if (0 == name_length) {
    errno = -EINVAL;
    return -1;
  }
  int value_length = strlen(value);

  struct env *p = internal_getenv(name);
  if (p) {
    if (!overwrite) {
      return 0;
    }
    char *new_ptr = realloc(p->value, value_length + 1);
    if (!new_ptr) {
      errno = -ENOMEM;
      return -1;
    }
    p->value = new_ptr;
    strcpy(p->value, value);
    return 0;
  }

  struct env *new_env = malloc(sizeof(struct env));
  if (!new_env) {
    return -ENOMEM;
  }
  new_env->name = malloc(name_length + 1);
  if (!new_env->name) {
    free(new_env);
    return -ENOMEM;
  }
  new_env->value = malloc(value_length + 1);
  if (!new_env->value) {
    free(new_env->name);
    free(new_env);
    return -ENOMEM;
  }
  strcpy(new_env->name, name);
  strcpy(new_env->value, value);
  new_env->next = env_head;
  env_head = new_env;
  return 0;
}
