#include <stdint.h>

// struct entry;
// struct stack;

struct entry {
  void *ptr;
  // TODO: Maybe don't use a linkedlist
  struct entry *next;
};

struct stack {
  struct entry *head;
};

void stack_init(struct stack *s);
int stack_push(struct stack *s, void *data);
void *stack_pop(struct stack *s);
