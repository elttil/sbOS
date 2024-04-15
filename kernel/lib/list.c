#include <assert.h>
#include <kmalloc.h>
#include <lib/list.h>

void list_init(struct list *list) {
  list->capacity = 5;
  list->tail_index = -1;
  list->entries = kmalloc(sizeof(void *) * list->capacity);
  if (!list->entries) {
    list->capacity = 0;
  }
}

void list_reset(struct list *list) {
  list->tail_index = -1;
}

int list_clone(struct list *in, struct list *out) {
  list_init(out);
  for (int i = 0;; i++) {
    void *output;
    if (!list_get(in, i, &output)) {
      break;
    }
    if (!list_add(out, output, NULL)) {
      list_free(out);
      return 0;
    }
  }
  return 1;
}

int list_add(struct list *list, void *entry, int *index) {
  if (list->tail_index + 1 >= list->capacity) {
    int new_capacity = list->capacity + 25;
    void *new_allocation =
        krealloc(list->entries, sizeof(void *) * new_capacity);
    if (!new_allocation) {
      return 0;
    }
    list->entries = new_allocation;
    list->capacity = new_capacity;
  }
  list->tail_index++;
  list->entries[list->tail_index] = entry;
  if (index) {
    *index = list->tail_index;
  }
  return 1;
}

int list_set(struct list *list, int index, void *entry) {
  if (index > list->tail_index) {
    assert(0);
    return 0;
  }
  list->entries[index] = entry;
  return 1;
}

int list_get(const struct list *list, int index, void **out) {
  if (index > list->tail_index) {
    return 0;
  }
  *out = list->entries[index];
  return 1;
}

void list_free(struct list *list) {
  list->capacity = 0;
  list->tail_index = -1;
  kfree(list->entries);
  list->entries = NULL;
}
