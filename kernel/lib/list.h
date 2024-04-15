#ifndef LIST_H
#define LIST_H
struct list {
  void **entries;
  int capacity;
  int tail_index;
};

void list_init(struct list *list);
void list_reset(struct list *list);
void list_free(struct list *list);
int list_clone(struct list *in, struct list *out);
int list_add(struct list *list, void *entry, int *index);
int list_set(struct list *list, int index, void *entry);
int list_get(const struct list *list, int index, void **out);
#endif
