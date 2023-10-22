#include "stdio.h"
#include "stdlib.h"
#include "../errno.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define NEW_ALLOC_SIZE 0x1000

#define IS_FREE (1 << 0)
#define IS_FINAL (1 << 1)

typedef struct MallocHeader {
  uint32_t size;
  uint8_t flags;
} MallocHeader;

MallocHeader *head = NULL;
MallocHeader *final = NULL;
uint32_t total_heap_size = 0;

int init_heap(void) {
  head = sbrk(NEW_ALLOC_SIZE);
  if ((void *)-1 == head) {
    // perror("sbrk");
    return 0;
  }
  total_heap_size += NEW_ALLOC_SIZE;
  head->size = NEW_ALLOC_SIZE;
  head->flags = IS_FREE | IS_FINAL;
  final = head;
  return 1;
}

int add_heap_memory(void) {
  if ((void *)-1 == sbrk(NEW_ALLOC_SIZE)) {
    // perror("sbrk");
    return 0;
  }
  total_heap_size += NEW_ALLOC_SIZE;
  final->size += NEW_ALLOC_SIZE;
  return 1;
}

MallocHeader *next_header(MallocHeader *a) {
  if (a->flags & IS_FINAL)
    return NULL;
  return ((void *)a) + a->size;
}

MallocHeader *find_free_entry(uint32_t s) {
  // A new header is required as well as the newly allocated chunk
  s += sizeof(MallocHeader);
  if (!head)
    init_heap();
  MallocHeader *p = head;
  for (; p; p = next_header(p)) {
    if (!(p->flags & IS_FREE))
      continue;
    if (p->size < s)
      continue;
    return p;
  }
  return NULL;
}

void merge_headers(MallocHeader *b) {
  if (!(b->flags & IS_FREE))
    return;

  MallocHeader *n = next_header(b);
  if (!n)
    return;

  if (!(n->flags & IS_FREE))
    return;

  // Remove the next header and just increase the newly freed header
  b->size += n->size;
  b->flags |= n->flags & IS_FINAL;
  if (b->flags & IS_FINAL)
    final = b;
}

extern int errno;
// https://pubs.opengroup.org/onlinepubs/9699919799/
void *malloc(size_t s) {
  if(s == 0)
      s = 1;

  MallocHeader *free_entry = find_free_entry(s);
  if (!free_entry) {
    if (!add_heap_memory()) {
        errno = ENOMEM;
      return NULL;
    }
    return malloc(s);
  }

  // Create a new header
  MallocHeader *new_entry = ((void *)free_entry) + s;
  new_entry->flags |= IS_FREE;
  new_entry->size = free_entry->size - s - sizeof(MallocHeader);
  new_entry->flags |= free_entry->flags & IS_FINAL;
  if (new_entry->flags & IS_FINAL)
    final = new_entry;
  merge_headers(new_entry);

  // Modify the free entry
  free_entry->size = s;
  free_entry->flags = 0;

  return ((void *)free_entry) + sizeof(MallocHeader);
}

// https://pubs.opengroup.org/onlinepubs/9699919799/
void *calloc(size_t nelem, size_t elsize) {
    // The calloc() function shall allocate unused space for an array of
    // nelem elements each of whose size in bytes is elsize.
    size_t alloc_size = nelem*elsize;
    void *rc = malloc(alloc_size);
    // The space shall be initialized to all bits 0.
    memset(rc, 0, alloc_size);
    return rc;
}

void free(void *p) {
  // FIXME: This assumes that p is at the start of a allocated area.
  // Is this a assumption that can be made?
  MallocHeader *h = p - sizeof(MallocHeader);
  if (h->flags & IS_FREE)
    return;

  h->flags |= IS_FREE;
  merge_headers(h);
}
