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
