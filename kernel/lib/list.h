#ifndef LIST_H
#define LIST_H
struct list {
  void **entries;
  int tail_index;
};

int list_init(struct list *list);
void list_reset(struct list *list);
void list_free(struct list *list);
int list_add(struct list *list, void *entry);
int list_get(const struct list *list, int index, void **out);
#endif
