#ifndef RELIST_H
#define RELIST_H
#include <typedefs.h>

struct relist {
  void **entries;
  u32 bitmap_capacity;
  u64 *bitmap;
};

void relist_init(struct relist *list);
void relist_reset(struct relist *list);
void relist_free(struct relist *list);
int relist_clone(struct relist *in, struct relist *out);
int relist_add(struct relist *list, void *value, u32 *index);
int relist_set(struct relist *list, u32 index, void *value);
int relist_get(const struct relist *list, u32 index, void **out, int *end);
int relist_remove(struct relist *list, u32 index);
#endif
