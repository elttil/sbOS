#include <assert.h>
#include <kmalloc.h>
#include <lib/list.h>

int list_init(struct list *list) {
  // TODO: Make it dynamic
  list->capacity = 5;
  list->entries = kmalloc(sizeof(void *) * list->capacity);
  if (!list->entries) {
    return 0;
  }
  list->tail_index = -1;
  return 1;
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
    list_add(out, output, NULL);
  }
  return 1;
}

int list_add(struct list *list, void *entry, int *index) {
  if (list->tail_index + 1 >= list->capacity) {
    list->capacity += 25;
    list->entries = krealloc(list->entries, sizeof(void *) * list->capacity);
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
  kfree(list->entries);
}
