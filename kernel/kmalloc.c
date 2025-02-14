#include <assert.h>
#include <interrupts.h>
#include <kmalloc.h>
#include <log.h>
#include <math.h>
#include <mmu.h>
#include <random.h>
#include <stdint.h>
#define NEW_ALLOC_SIZE 0x40000

// #define KMALLOC_DEBUG

#define IS_FREE (1 << 0)
#define IS_FINAL (1 << 1)

void *kmalloc_align(size_t s, void **physical) {
  // TODO: It should reuse virtual regions so that it does not run out
  // of address space.
  void *rc;
  if (!(rc = ksbrk_physical(s, physical))) {
    return NULL;
  }
  return rc;
}

void kmalloc_align_free(void *p, size_t s) {
  if (!p) {
    return;
  }

  for (size_t i = 0; i < s; i += 0x1000) {
    Page *page = get_page((char *)p + i, NULL, PAGE_NO_ALLOCATE, 0);
    if (!page) {
      continue;
    }
    if (!page->present) {
      continue;
    }
    if (!page->frame) {
      continue;
    }
    write_to_frame(((u32)page->frame) * 0x1000, 0);
    page->present = 0;
  }
  flush_tlb();
}

typedef struct MallocHeader {
  u64 magic;
  u32 size;
  u8 flags;
  struct MallocHeader *n;
} MallocHeader;

u64 delta_page(u64 a) {
  return 0x1000 - (a % 0x1000);
}

MallocHeader *head = NULL;
MallocHeader *final = NULL;
u32 total_heap_size = 0;

int init_heap(void) {
  head = (MallocHeader *)ksbrk(NEW_ALLOC_SIZE, 0);
  if (!head) {
    return 0;
  }
  total_heap_size += NEW_ALLOC_SIZE - sizeof(MallocHeader);
  head->magic = 0xdde51ab9410268b1;
  head->size = NEW_ALLOC_SIZE - sizeof(MallocHeader);
  head->flags = IS_FREE | IS_FINAL;
  head->n = NULL;
  final = head;
  return 1;
}

int add_heap_memory(size_t min_desired) {
  min_desired += sizeof(MallocHeader);
  size_t allocation_size = max(min_desired, NEW_ALLOC_SIZE);
  allocation_size += delta_page(allocation_size);
  allocation_size += NEW_ALLOC_SIZE;
  void *p;
  if (!(p = (void *)ksbrk(allocation_size, 0))) {
    return 0;
  }
  total_heap_size += allocation_size - sizeof(MallocHeader);
  if (IS_FREE & final->flags) {
    uintptr_t e = final;
    e = ((uintptr_t)e + sizeof(MallocHeader) + final->size);
    if (p == (void *)e) {
      final->size += allocation_size - sizeof(MallocHeader);
      return 1;
    }
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

static MallocHeader *next_header(MallocHeader *a) {
  assert(a->magic == 0xdde51ab9410268b1);
  if (a->n) {
    if (a->n->magic != 0xdde51ab9410268b1) {
      kprintf("Real magic value is: %x\n", a->n->magic);
      kprintf("location: %x\n", &(a->n->magic));
      assert(0);
    }
    return a->n;
  }
  return NULL;
}

void kmalloc_scan(void) {
  if (!head) {
    return;
  }
  MallocHeader *p = head;
  for (; (p = next_header(p));)
    ;
}

static MallocHeader *next_close_header(MallocHeader *a) {
  assert(a);
  if (a->flags & IS_FINAL) {
    return NULL;
  }
  return next_header(a);
}

int merge_headers(MallocHeader *b);

static MallocHeader *find_free_entry(u32 s) {
  // A new header is required as well as the newly allocated chunk
  s += sizeof(MallocHeader);
  if (!head) {
    if (!init_heap()) {
      return NULL;
    }
  }
  MallocHeader *p = head;
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
  if (n == final) {
    final = b;
  }
  return 1;
}

#ifdef KMALLOC_DEBUG
void *int_kmalloc(size_t s) {
  disable_interrupts();
  u8 *rc = kmalloc_align(s, NULL);
  get_fast_insecure_random(rc, s);
  rc += align_page(s);
  rc -= s;

  void *delay = kmalloc_align(1, NULL);
  kmalloc_align_free(delay, 1);
  return (void *)rc;
}

void kfree(void *p) {
  get_fast_insecure_random(align_page(p) - 0x1000, 0x1000);
  kmalloc_align_free(p, 0x1000);
}
#else
void *int_kmalloc(size_t s) {
  size_t n = s;
  MallocHeader *free_entry = find_free_entry(s);
  if (!free_entry) {
    if (!add_heap_memory(s)) {
      klog(LOG_ERROR, "Ran out of memory.");
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

  if (free_entry == final) {
    final = new_entry;
  }
  merge_headers(new_entry);

  // Modify the free entry
  free_entry->size = n;
  free_entry->flags = 0;
  free_entry->n = new_entry;
  free_entry->magic = 0xdde51ab9410268b1;
  return rc;
}

void kfree(void *p) {
  if (!p) {
    return;
  }

  MallocHeader *h = (MallocHeader *)((uintptr_t)p - sizeof(MallocHeader));
  assert(h->magic == 0xdde51ab9410268b1);
  assert(!(h->flags & IS_FREE));

  get_fast_insecure_random((void *)p, h->size);

  h->flags |= IS_FREE;
  merge_headers(h);
}
#endif // KMALLOC_DEBUG

void *kmalloc(size_t s) {
  void *rc = int_kmalloc(s);
  if (NULL == rc) {
    return NULL;
  }
  get_fast_insecure_random((void *)rc, s);
  return rc;
}

size_t get_mem_size(void *ptr) {
  if (!ptr) {
    return 0;
  }
  return ((MallocHeader *)((uintptr_t)ptr - sizeof(MallocHeader)))->size;
}

void *krealloc(void *ptr, size_t size) {
  if (!ptr) {
    return kmalloc(size);
  }
  size_t l = get_mem_size(ptr);
  if (l == size) {
    return ptr;
  }
  if (l > size) {
    MallocHeader *header = (MallocHeader *)((u8 *)ptr - sizeof(MallocHeader));
    header->size = size;
    return ptr;
  }

  void *rc = kmalloc(size);
  if (!rc) {
    return NULL;
  }
  size_t to_copy = min(l, size);
  memcpy(rc, ptr, to_copy);
  kfree(ptr);
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

void *krecalloc(void *ptr, size_t nelem, size_t elsize) {
  if ((nelem >= MUL_NO_OVERFLOW || elsize >= MUL_NO_OVERFLOW) && nelem > 0 &&
      SIZE_MAX / nelem < elsize) {
    return NULL;
  }
  if (!ptr) {
    return kcalloc(nelem, elsize);
  }
  size_t new_size = nelem * elsize;
  void *rc = int_kmalloc(new_size);
  if (!rc) {
    return NULL;
  }
  size_t l = get_mem_size(ptr);
  size_t to_copy = min(l, new_size);
  memset(rc, 0, new_size);
  memcpy(rc, ptr, to_copy);
  kfree(ptr);
  return rc;
}

void *kcalloc(size_t nelem, size_t elsize) {
  if ((nelem >= MUL_NO_OVERFLOW || elsize >= MUL_NO_OVERFLOW) && nelem > 0 &&
      SIZE_MAX / nelem < elsize) {
    return NULL;
  }
  void *rc = int_kmalloc(nelem * elsize);
  if (!rc) {
    return NULL;
  }
  memset(rc, 0, nelem * elsize);
  return rc;
}
