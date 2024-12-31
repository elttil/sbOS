#include "handle.h"
#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <unistd.h>

struct handle {
  int fd;
  char *key;
  char *pathname;
  struct handle *next;
  struct handle *prev;
};

struct handle *head = NULL;

char *generate_random_key(int length) {
  assert(length < 256);
  char buf[256];

  randomfill(buf, 256);
  //  assert(0 == getentropy(buf, 256));
  char *r = malloc(length + 1);
  if (!r) {
    return NULL;
  }
  for (int i = 0; i < length; i++) {
    r[i] = 'A' + (buf[i] & 0xF);
  }
  r[length] = '\0';
  return r;
}

static struct handle *handle_get(struct sv key) {
  struct handle *p = head;
  for (; p; p = p->next) {
    if (!sv_eq(C_TO_SV(p->key), key)) {
      continue;
    }
    return p;
  }
  return NULL;
}

int handle_get_fd(struct sv key, int *fd, char **pathname) {
  struct handle *h = handle_get(key);
  if (!h) {
    return 0;
  }
  if (fd) {
    *fd = h->fd;
  }
  if (pathname) {
    *pathname = h->pathname;
  }
  return 1;
}

char *handle_create(struct sv path, int is_dir, int flags, int mode) {
  int f;
  if (is_dir) {
    //    f = O_DIRECTORY | O_RDONLY;
    f = O_RDONLY;
  } else {
    f = flags;
  }
  char *p = SV_TO_C(path);
  int fd = open(p, f, mode);
  if (-1 == fd) {
    free(p);
    return NULL;
  }
  free(p);

  // 64 bits of entropy
  char *key = generate_random_key(16);
  if (!key) {
    close(fd);
    return NULL;
  }

  struct handle *n = malloc(sizeof(struct handle));
  if (!n) {
    free(key);
    close(fd);
    return NULL;
  }
  n->fd = fd;
  n->key = key;
  n->pathname = SV_TO_C(path);
  n->next = head;
  n->prev = NULL;
  if (head) {
    head->prev = n;
  }
  head = n;
  return key;
}

int handle_close(struct sv key) {
  struct handle *h = handle_get(key);
  if (!h) {
    return 0;
  }

  if (head == h) {
    head = h->next;
  }

  if (h->prev) {
    h->prev->next = h->next;
  }
  if (h->next) {
    h->next->prev = h->prev;
  }
  free(h->key);
  free(h->pathname);
  free(h);
  return 1;
}
