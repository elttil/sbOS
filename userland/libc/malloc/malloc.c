#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <typedefs.h>
#include <unistd.h>
#define NEW_ALLOC_SIZE 0x20000

#define IS_FREE (1 << 0)
#define IS_FINAL (1 << 1)

typedef struct MallocHeader {
  u64 magic;
  u32 size;
  u8 flags;
  struct MallocHeader *n;
} MallocHeader;

u64 delta_page(u64 a) {
  return 0x1000 - (a % 0x1000);
}

MallocHeader *malloc_head = NULL;
MallocHeader *malloc_final = NULL;
u32 total_heap_size = 0;

// printf without using malloc() so that it can be used internally by
// malloc() such that it does not have a stack overflow.
int debug_vprintf(const char *fmt, va_list ap) {
  char buffer[4096];
  int rc = vsnprintf(buffer, 4096, fmt, ap);
  if (0 > rc) {
    return -1;
  }
  write(1, buffer, rc);

  static int serial_fd = -1;
  if (-1 == serial_fd) {
    serial_fd = open("/dev/serial", O_RDWR);
  }
  write(serial_fd, buffer, rc);
  return rc;
}

int debug_printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int rc = debug_vprintf(fmt, ap);
  va_end(ap);
  return rc;
}

int init_heap(void) {
  malloc_head = (MallocHeader *)sbrk(NEW_ALLOC_SIZE);
  total_heap_size += NEW_ALLOC_SIZE - sizeof(MallocHeader);
  malloc_head->magic = 0xdde51ab9410268b1;
  malloc_head->size = NEW_ALLOC_SIZE - sizeof(MallocHeader);
  malloc_head->flags = IS_FREE | IS_FINAL;
  malloc_head->n = NULL;
  malloc_final = malloc_head;
  return 1;
}

int add_heap_memory(size_t min_desired) {
  min_desired += sizeof(MallocHeader) + 0x1000;
  size_t allocation_size = max(min_desired, NEW_ALLOC_SIZE);
  allocation_size += delta_page(allocation_size);
  void *p;
  if ((void *)(-1) == (p = (void *)sbrk(allocation_size))) {
    return 0;
  }
  total_heap_size += allocation_size - sizeof(MallocHeader);
  void *e = malloc_final;
  e = (void *)((u32)e + malloc_final->size);
  if (p == e) {
    malloc_final->size += allocation_size - sizeof(MallocHeader);
    return 1;
  }
  MallocHeader *new_entry = p;
  new_entry->size = allocation_size - sizeof(MallocHeader);
  new_entry->flags = IS_FREE | IS_FINAL;
  new_entry->n = NULL;
  new_entry->magic = 0xdde51ab9410268b1;
  malloc_final->n = new_entry;
  malloc_final = new_entry;
  return 1;
}

MallocHeader *next_header(MallocHeader *a) {
  assert(a->magic == 0xdde51ab9410268b1);
  if (a->n) {
    if (a->n->magic != 0xdde51ab9410268b1) {
      debug_printf("a->n: %x\n", a->n);
      debug_printf("Real magic value is: %x\n", a->n->magic);
      debug_printf("size: %x\n", a->n->size);
      debug_printf("location: %x\n", &(a->n->magic));
      assert(0);
    }
    return a->n;
  }
  return NULL;
}

MallocHeader *next_close_header(MallocHeader *a) {
  if (!a) {
    debug_printf("next close header fail\n");
    for (;;)
      ;
  }
  if (a->flags & IS_FINAL) {
    return NULL;
  }
  return next_header(a);
}

int merge_headers(MallocHeader *b);
MallocHeader *find_free_entry(u32 s) {
  // A new header is required as well as the newly allocated chunk
  s += sizeof(MallocHeader);
  if (!malloc_head) {
    init_heap();
  }
  MallocHeader *p = malloc_head;
  for (; p; p = next_header(p)) {
    assert(p->magic == 0xdde51ab9410268b1);
    if (!(p->flags & IS_FREE)) {
      continue;
    }
    u64 required_size = s;
    if (p->size < required_size) {
      for (; merge_headers(p);)
        ;
      if (p->size >= required_size) {
        return p;
      }
      continue;
    }
    return p;
  }
  return NULL;
}

int merge_headers(MallocHeader *b) {
  if (!(b->flags & IS_FREE)) {
    return 0;
  }

  MallocHeader *n = next_close_header(b);
  if (!n) {
    return 0;
  }

  if (!(n->flags & IS_FREE)) {
    return 0;
  }

  b->size += n->size;
  b->flags |= n->flags & IS_FINAL;
  b->n = n->n;
  if (n == malloc_final) {
    malloc_final = b;
  }
  return 1;
}

void *int_malloc(size_t s, int recursion) {
  size_t n = s;
  MallocHeader *free_entry = find_free_entry(s);
  if (!free_entry) {
    if (!add_heap_memory(s)) {
      assert(0);
      return NULL;
    }
    if (recursion) {
      debug_printf("RECURSION IN MALLOC :(\n");
      assert(0);
    }
    return int_malloc(s, 1);
  }

  void *rc = (void *)(free_entry + 1);

  // Create a new header
  MallocHeader *new_entry = (MallocHeader *)((uintptr_t)rc + n);
  new_entry->flags = free_entry->flags;
  new_entry->n = free_entry->n;
  new_entry->size = free_entry->size - n - sizeof(MallocHeader);
  new_entry->magic = 0xdde51ab9410268b1;

  if (free_entry == malloc_final) {
    malloc_final = new_entry;
  }
  merge_headers(new_entry);

  // Modify the free entry
  free_entry->size = n;
  free_entry->flags = 0;
  free_entry->n = new_entry;
  free_entry->magic = 0xdde51ab9410268b1;
  randomfill(rc, s);
  return rc;
}

void *malloc(size_t s) {
  return int_malloc(s, 0);
}

size_t get_mem_size(void *ptr) {
  if (!ptr) {
    return 0;
  }
  return ((MallocHeader *)((uintptr_t)ptr - sizeof(MallocHeader)))->size;
}

void *realloc(void *ptr, size_t size) {
  void *rc = malloc(size);
  if (!rc) {
    return NULL;
  }
  if (!ptr) {
    return rc;
  }
  size_t l = get_mem_size(ptr);
  size_t to_copy = min(l, size);
  memcpy(rc, ptr, to_copy);
  free(ptr);
  return rc;
}

// This is sqrt(SIZE_MAX+1), as s1*s2 <= SIZE_MAX
// if both s1 < MUL_NO_OVERFLOW and s2 < MUL_NO_OVERFLOW
#define MUL_NO_OVERFLOW ((size_t)1 << (sizeof(size_t) * 4))

void *reallocarray(void *ptr, size_t nmemb, size_t size) {
  if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) && nmemb > 0 &&
      SIZE_MAX / nmemb < size) {
    return NULL;
  }

  return realloc(ptr, nmemb * size);
}

void *allocarray(size_t nmemb, size_t size) {
  return reallocarray(NULL, nmemb, size);
}

void *calloc(size_t nelem, size_t elsize) {
  void *rc = allocarray(nelem, elsize);
  if (!rc) {
    return NULL;
  }
  memset(rc, 0, nelem * elsize);
  return rc;
}

void free(void *p) {
  if (!p) {
    return;
  }
  MallocHeader *h = (MallocHeader *)((uintptr_t)p - sizeof(MallocHeader));
  assert(h->magic == 0xdde51ab9410268b1);
  assert(!(h->flags & IS_FREE));

  randomfill(p, h->size);

  h->flags |= IS_FREE;
  merge_headers(h);
}
