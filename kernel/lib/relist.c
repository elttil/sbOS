#include <assert.h>
#include <kmalloc.h>
#include <lib/relist.h>

void relist_init(struct relist *list) {
  list->bitmap_capacity = 100;
  list->bitmap = kcalloc(sizeof(u64), list->bitmap_capacity);
  if (!list->bitmap) {
    goto relist_init_error;
  }
  list->entries = kallocarray(sizeof(void *), (sizeof(u64) * 8));
  if (!list->entries) {
    goto relist_init_error;
  }
  return;

// Can't allocate now but future appends can try to allocate anyways so
// there is no reason to "fail" the init.
relist_init_error:
  list->bitmap_capacity = 0;
  list->entries = NULL;
  list->bitmap = NULL;
  return;
}

void relist_reset(struct relist *list) {
  memset(list->bitmap, 0, list->bitmap_capacity * sizeof(u64));
}

int relist_clone(struct relist *in, struct relist *out) {
  relist_init(out);
  for (int i = 0;; i++) {
    void *output;
    if (!relist_get(in, i, &output)) {
      break;
    }
    if (!relist_add(out, output, NULL)) {
      relist_free(out);
      return 0;
    }
  }
  return 1;
}

static int relist_find_free_entry(struct relist *list, u32 *entry) {
  for (u32 i = 0; i < list->bitmap_capacity; i++) {
    if (0 == ~(list->bitmap[i])) {
      continue;
    }

    for (int c = 0; c < 64; c++) {
      if (!(list->bitmap[i] & ((u64)1 << c))) {
        *entry = i * 64 + c;
        return 1;
      }
    }
  }
  *entry = 0;
  return 0;
}

int relist_add(struct relist *list, void *value, u32 *index) {
  u32 entry;
  if (!relist_find_free_entry(list, &entry)) {
    assert(0);
  }
  list->entries[entry] = value;
  if (index) {
    *index = entry;
  }
  list->bitmap[entry / 64] |= ((u64)1 << (entry % 64));
  return 1;
}

int relist_remove(struct relist *list, u32 index) {
  if ((index / 64) > list->bitmap_capacity) {
    assert(0);
    return 0;
  }
  int is_used = (list->bitmap[index / 64] & ((u64)1 << (index % 64)));
  if (!is_used) {
    assert(0);
    return 0;
  }
  list->bitmap[index / 64] &= ~((u64)1 << (index % 64));
  return 1;
}

int relist_set(struct relist *list, u32 index, void *value) {
  assert(value);
  if ((index / 64) > list->bitmap_capacity) {
    assert(0);
    return 0;
  }
  int is_used = (list->bitmap[index / 64] & ((u64)1 << (index % 64)));
  assert(is_used);
  list->entries[index] = value;
  return 1;
}

int relist_get(const struct relist *list, u32 index, void **out) {
  if ((index / 64) > list->bitmap_capacity) {
    assert(0);
    return 0;
  }
  int is_used = (list->bitmap[index / 64] & ((u64)1 << (index % 64)));
  if (!is_used) {
    return 0;
  }
  *out = list->entries[index];
  return 1;
}

void relist_free(struct relist *list) {
  list->bitmap_capacity = 0;
  kfree(list->entries);
  kfree(list->bitmap);
  list->entries = NULL;
  list->bitmap = NULL;
}
