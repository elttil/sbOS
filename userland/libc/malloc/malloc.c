#include <assert.h>
#include <errno.h>
#include <malloc/malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define NEW_ALLOC_SIZE 0x20000
#define MALLOC_HEADER_MAGIC 0x1337F00D
#define MALLOC_HEADER_PAD 0x11223344

#define IS_FREE (1 << 0)
#define IS_FINAL (1 << 1)

typedef struct MallocHeader {
  uint32_t magic;
  uint32_t size;
  uint8_t flags;
  struct MallocHeader *n;
} MallocHeader;

size_t max(size_t a, size_t b) { return (a > b) ? a : b; }

//;size_t min(size_t a, size_t b) { return (a < b) ? a : b; }

uint64_t delta_page(uint64_t a) { return 0x1000 - (a % 0x1000); }

MallocHeader *head = NULL;
MallocHeader *final = NULL;
uint32_t total_heap_size = 0;

int init_heap(void) {
  head = (MallocHeader *)sbrk(NEW_ALLOC_SIZE);
  if ((void *)-1 == head) {
    perror("sbrk");
    exit(1);
  }
  total_heap_size += NEW_ALLOC_SIZE - sizeof(MallocHeader);
  head->magic = MALLOC_HEADER_MAGIC;
  head->size = NEW_ALLOC_SIZE - sizeof(MallocHeader);
  head->flags = IS_FREE | IS_FINAL;
  head->n = NULL;
  final = head;
  return 1;
}

int add_heap_memory(size_t min_desired) {
  min_desired += sizeof(MallocHeader) + 0x1000;
  size_t allocation_size = max(min_desired, NEW_ALLOC_SIZE);
  allocation_size += delta_page(allocation_size);
  void *p;
  if ((void *)(-1) == (p = (void *)sbrk(allocation_size))) {
    perror("sbrk");
    return 0;
  }
  total_heap_size += allocation_size - sizeof(MallocHeader);
  void *e = final;
  e = (void *)((uint32_t)e + final->size);
  if (p == e) {
    final->size += allocation_size - sizeof(MallocHeader);
    return 1;
  }
  MallocHeader *new_entry = p;
  new_entry->magic = MALLOC_HEADER_MAGIC;
  new_entry->size = allocation_size - sizeof(MallocHeader);
  new_entry->flags = IS_FREE | IS_FINAL;
  new_entry->n = NULL;
  final->n = new_entry;
  final = new_entry;
  return 1;
}

MallocHeader *next_header(MallocHeader *a) { return a->n; }

MallocHeader *next_close_header(MallocHeader *a) {
  if (!a) {
    printf("next close header fail\n");
    for (;;)
      ;
  }
  if (a->flags & IS_FINAL)
    return NULL;
  return next_header(a);
}

MallocHeader *find_free_entry(uint32_t s, int align) {
  // A new header is required as well as the newly allocated chunk
  if (!head)
    init_heap();
  MallocHeader *p = head;
  for (; p; p = next_header(p)) {
    if (!(p->flags & IS_FREE))
      continue;
    uint64_t required_size = s;
    if (align) {
      void *addy = p;
      addy = (void *)((uint32_t)addy + sizeof(MallocHeader));
      uint64_t d = delta_page((uint32_t)addy);
      if (d < sizeof(MallocHeader) && d != 0)
        continue;
      required_size += d;
    }
    if (p->size < required_size)
      continue;
    return p;
  }
  return NULL;
}

void merge_headers(MallocHeader *b) {
  if (!(b->flags & IS_FREE))
    return;

  MallocHeader *n = next_close_header(b);
  if (!n)
    return;

  if (n > 0xf58c0820 - 0x8 && n < 0xf58c0820 + 0x8) {
    printf("b: %x\n", b);
    printf("b->n: %x\n", b->n);
    asm("hlt");
    assert(0);
  }
  if (!(n->flags & IS_FREE))
    return;

  // Remove the next header and just increase the newly freed header
  b->size += n->size;
  b->flags |= n->flags & IS_FINAL;
  b->n = n->n;
  if (n == final)
    final = b;
}

extern int errno;
// https://pubs.opengroup.org/onlinepubs/9699919799/
void *int_malloc(size_t s, int align) {
  if (!head)
    init_heap();
  size_t n = s;
  MallocHeader *free_entry = find_free_entry(s, align);
  if (!free_entry) {
    if (!add_heap_memory(s)) {
      errno = ENOMEM;
      printf("ENOMEM\n");
      return NULL;
    }
    return int_malloc(s, align);
  }

  void *rc = (void *)((uint32_t)free_entry + sizeof(MallocHeader));

  if (align) {
    uint64_t d = delta_page((uint32_t)rc);
    n = d;
    n -= sizeof(MallocHeader);
  }

  // Create a new header
  MallocHeader *new_entry =
      (MallocHeader *)((uint32_t)free_entry + n + sizeof(MallocHeader));
  new_entry->magic = MALLOC_HEADER_MAGIC;
  new_entry->flags = free_entry->flags;
  new_entry->n = free_entry->n;
  new_entry->size = free_entry->size - n - sizeof(MallocHeader);
  if (free_entry == final)
    final = new_entry;
  merge_headers(new_entry);

  // Modify the free entry
  free_entry->size = n;
  free_entry->flags = 0;
  free_entry->n = new_entry;

  if (align && ((uint32_t)rc % 0x1000) != 0) {
    void *c = int_malloc(s, 1);
    free(rc);
    rc = c;
    return rc;
  }
  return rc;
}

void *malloc(size_t s) { return int_malloc(s + 1, 0); }

// https://pubs.opengroup.org/onlinepubs/9699919799/
void *calloc(size_t nelem, size_t elsize) {
  // The calloc() function shall allocate unused space for an array of
  // nelem elements each of whose size in bytes is elsize.
  size_t alloc_size = nelem * elsize;
  void *rc = malloc(alloc_size);
  // The space shall be initialized to all bits 0.
  memset(rc, 0, alloc_size);
  return rc;
}

size_t get_mem_size(void *ptr) {
  if (!ptr)
    return 0;
  return ((MallocHeader *)(ptr - sizeof(MallocHeader)))->size;
}

void *realloc(void *ptr, size_t size) {
  void *rc = malloc(size);
  if (!rc)
    return NULL;
  size_t l = get_mem_size(ptr);
  size_t to_copy = min(l, size);
  memcpy(rc, ptr, to_copy);
  free(ptr);
  return rc;
}

void free(void *p) {
  if (!p)
    return;
  // FIXME: This assumes that p is at the start of a allocated area.
  // Is this a assumption that can be made?
  MallocHeader *h = (MallocHeader *)((uint32_t)p - sizeof(MallocHeader));
  if (MALLOC_HEADER_MAGIC != h->magic) {
#ifdef LIBC_DEBUG
    printf("LibC Malloc: free() is attempted at the incorrect location or at a corrupted header.\n");
    printf("h->magic: %x\n", h->magic);
    printf("&h->magic: %x\n", &(h->magic));
#endif
    return;
  }
  if (h->flags & IS_FREE)
    return;

  h->flags |= IS_FREE;
  merge_headers(h);
}
