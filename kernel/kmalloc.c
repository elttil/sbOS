#include <assert.h>
#include <kmalloc.h>
#include <ksbrk.h>
#include <math.h>
#include <random.h>
#define NEW_ALLOC_SIZE 0x20000

#define IS_FREE (1 << 0)
#define IS_FINAL (1 << 1)

typedef struct MallocHeader {
  uint64_t magic;
  uint32_t size;
  uint8_t flags;
  struct MallocHeader *n;
} MallocHeader;

uint64_t delta_page(uint64_t a) { return 0x1000 - (a % 0x1000); }

MallocHeader *head = NULL;
MallocHeader *final = NULL;
uint32_t total_heap_size = 0;

int init_heap(void) {
  head = (MallocHeader *)ksbrk(NEW_ALLOC_SIZE);
  total_heap_size += NEW_ALLOC_SIZE - sizeof(MallocHeader);
  head->magic = 0xdde51ab9410268b1;
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
  if ((void *)(-1) == (p = (void *)ksbrk(allocation_size))) {
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
  new_entry->size = allocation_size - sizeof(MallocHeader);
  new_entry->flags = IS_FREE | IS_FINAL;
  new_entry->n = NULL;
  new_entry->magic = 0xdde51ab9410268b1;
  final->n = new_entry;
  final = new_entry;
  return 1;
}

MallocHeader *next_header(MallocHeader *a) {
  assert(a->magic == 0xdde51ab9410268b1);
  if (a->n) {
    assert(a->n->magic == 0xdde51ab9410268b1);
    return a->n;
  }
  return NULL;
}

MallocHeader *next_close_header(MallocHeader *a) {
  if (!a) {
    kprintf("next close header fail\n");
    for (;;)
      ;
  }
  if (a->flags & IS_FINAL)
    return NULL;
  return next_header(a);
}

MallocHeader *find_free_entry(uint32_t s) {
  // A new header is required as well as the newly allocated chunk
  s += sizeof(MallocHeader);
  if (!head)
    init_heap();
  MallocHeader *p = head;
  for (; p; p = next_header(p)) {
    assert(p->magic == 0xdde51ab9410268b1);
    if (!(p->flags & IS_FREE))
      continue;
    uint64_t required_size = s;
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

  if (!(n->flags & IS_FREE))
    return;

  b->size += n->size;
  b->flags |= n->flags & IS_FINAL;
  b->n = n->n;
  if (n == final)
    final = b;
}

void *kmalloc(size_t s) {
  size_t n = s;
  MallocHeader *free_entry = find_free_entry(s);
  if (!free_entry) {
    if (!add_heap_memory(s)) {
      klog("Ran out of memory.", LOG_ERROR);
      assert(0);
      return NULL;
    }
    return kmalloc(s);
  }

  void *rc = (void *)(free_entry + 1);

  // Create a new header
  MallocHeader *new_entry = (MallocHeader *)((uintptr_t)rc + n);
  new_entry->flags = free_entry->flags;
  new_entry->n = free_entry->n;
  new_entry->size = free_entry->size - n - sizeof(MallocHeader);
  new_entry->magic = 0xdde51ab9410268b1;

  if (free_entry == final)
    final = new_entry;
  merge_headers(new_entry);

  // Modify the free entry
  free_entry->size = n;
  free_entry->flags = 0;
  free_entry->n = new_entry;
  free_entry->magic = 0xdde51ab9410268b1;
  get_random((void *)rc, s);
  return rc;
}

#define HEAP 0x00E00000
#define PHYS 0x403000

void *latest = NULL;
uint64_t left = 0;

void *kmalloc_eternal_physical_align(size_t s, void **physical) {
  void *return_address = ksbrk(s);
  if (physical) {
    if (0 == get_active_pagedirectory())
      *physical =
          (void *)((uintptr_t)return_address - (0xC0000000 + PHYS) + HEAP);
    else
      *physical = (void *)virtual_to_physical(return_address, 0);
  }
  memset(return_address, 0, 0x1000);
  return return_address;
}

void *kmalloc_eternal_align(size_t s) {
  return kmalloc_eternal_physical_align(s, NULL);
}

void *kmalloc_eternal(size_t s) { return kmalloc_eternal_align(s); }

size_t get_mem_size(void *ptr) {
  if (!ptr)
    return 0;
  return ((MallocHeader *)((uintptr_t)ptr - sizeof(MallocHeader)))->size;
}

void *krealloc(void *ptr, size_t size) {
  void *rc = kmalloc(size);
  if (!rc)
    return NULL;
  if (!ptr)
    return rc;
  size_t l = get_mem_size(ptr);
  size_t to_copy = min(l, size);
  memcpy(rc, ptr, to_copy);
  //  kfree(ptr);
  return rc;
}

// This is sqrt(SIZE_MAX+1), as s1*s2 <= SIZE_MAX
// if both s1 < MUL_NO_OVERFLOW and s2 < MUL_NO_OVERFLOW
#define MUL_NO_OVERFLOW ((size_t)1 << (sizeof(size_t) * 4))

void *kreallocarray(void *ptr, size_t nmemb, size_t size) {
  if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) && nmemb > 0 &&
      SIZE_MAX / nmemb < size) {
    return NULL;
  }

  return krealloc(ptr, nmemb * size);
}

void *kallocarray(size_t nmemb, size_t size) {
  return kreallocarray(NULL, nmemb, size);
}

void *kcalloc(size_t nelem, size_t elsize) {
  void *rc = kallocarray(nelem, elsize);
  if (!rc)
    return NULL;
  memset(rc, 0, nelem * elsize);
  return rc;
}

void kfree(void *p) {
  if (!p)
    return;
  // FIXME: This assumes that p is at the start of a allocated area.
  // Could this be avoided in a simple way?
  MallocHeader *h = (MallocHeader *)((uintptr_t)p - sizeof(MallocHeader));
  assert(h->magic == 0xdde51ab9410268b1);
  if (h->flags & IS_FREE)
    return;

  h->flags |= IS_FREE;
  merge_headers(h);
}
