#include <kmalloc.h>
#include <lib/stack.h>
#include <stddef.h>

void stack_init(struct stack *s) {
  s->head = NULL;
}

// 1 = Success
// 0 = Failure
int stack_push(struct stack *s, void *data) {
  struct entry *new_entry = kmalloc(sizeof(struct entry));
  if (NULL == new_entry) {
    return 0;
  }
  new_entry->ptr = data;

  new_entry->next = s->head;
  s->head = new_entry;
  return 1;
}

// Returns data on success
// Returns NULL on failure
void *stack_pop(struct stack *s) {
  struct entry *recieved_entry = s->head;
  if (NULL == recieved_entry) {
    return NULL;
  }
  s->head = recieved_entry->next;

  void *r = recieved_entry->ptr;
  kfree(recieved_entry);
  return r;
}
