#include <assert.h>
#include <kmalloc.h>
#include <lib/relist.h>
#include <math.h>

void relist_init(struct relist *list) {
  list->bitmap_capacity = 1;
  list->bitmap = kcalloc(sizeof(u64), list->bitmap_capacity);
  if (!list->bitmap) {
    goto relist_init_error;
  }
  list->entries =
      kallocarray(sizeof(void *) * sizeof(u64) * 8, list->bitmap_capacity);
  if (!list->entries) {
    kfree(list->bitmap);
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
  out->bitmap_capacity = in->bitmap_capacity;
  out->bitmap = kcalloc(sizeof(u64), out->bitmap_capacity);
  if (!out->bitmap) {
    goto relist_clone_error;
  }
  out->entries =
      kallocarray(sizeof(void *) * sizeof(u64) * 8, out->bitmap_capacity);
  if (!out->entries) {
    kfree(out->bitmap);
    goto relist_clone_error;
  }
  memcpy(out->bitmap, in->bitmap, sizeof(u64) * out->bitmap_capacity);
  memcpy(out->entries, in->entries,
         sizeof(void *) * sizeof(u64) * 8 * out->bitmap_capacity);
  return 1;

relist_clone_error:
  out->bitmap_capacity = 0;
  out->entries = NULL;
  out->bitmap = NULL;
  return 0;
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
    u32 new_capacity = list->bitmap_capacity + 1;
    u64 *new_allocation = kcalloc(sizeof(u64), new_capacity);
    if (!new_allocation) {
      return 0;
    }
    void *new_entry = krecalloc(list->entries, sizeof(void *) * sizeof(u64) * 8,
                                new_capacity);
    if (!new_entry) {
      kfree(new_allocation);
      return 0;
    }
    if (list->bitmap) {
      size_t to_copy = min(list->bitmap_capacity, new_capacity) * sizeof(u64);
      memcpy(new_allocation, list->bitmap, to_copy);
      kfree(list->bitmap);
    }
    list->bitmap = new_allocation;
    list->bitmap_capacity = new_capacity;
    list->entries = new_entry;
    assert(relist_find_free_entry(list, &entry));
  }
  list->entries[entry] = value;
  if (index) {
    *index = entry;
  }
  list->bitmap[entry / 64] |= ((u64)1 << (entry % 64));
  return 1;
}

int relist_remove(struct relist *list, u32 index) {
  if (index >= list->bitmap_capacity * 64) {
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
  if (index >= list->bitmap_capacity * 64) {
    assert(0);
    return 0;
  }
  int is_used = (list->bitmap[index / 64] & ((u64)1 << (index % 64)));
  assert(is_used);
  list->entries[index] = value;
  return 1;
}

int relist_get(const struct relist *list, u32 index, void **out, int *end) {
  if (end) {
    *end = 0;
  }
  if (index >= list->bitmap_capacity * 64) {
    if (end) {
      *end = 1;
    }
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
