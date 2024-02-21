#include <assert.h>
#include <kmalloc.h>
#include <lib/list.h>

int list_init(struct list *list) {
  // TODO: Make it dynamic
  list->entries = kmalloc(sizeof(void *) * 100);
  if (!list->entries) {
    return 0;
  }
  list->tail_index = -1;
  return 1;
}

void list_reset(struct list *list) {
  list->tail_index = -1;
}

int list_add(struct list *list, void *entry) {
  if (list->tail_index > 100 - 1) {
    kprintf("Error: list has run out of space\n");
    assert(0);
  }
  list->tail_index++;
  list->entries[list->tail_index] = entry;
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
