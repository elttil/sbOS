#include <assert.h>
#include <ksbrk.h>
#include <mmu.h>
#include <stddef.h>
#include <stdint.h>

/*
extern uintptr_t data_end;
extern PageDirectory *kernel_directory;

#define HEAP 0x00E00000
#define PHYS 0x403000
#define PAGE_SIZE ((uintptr_t)0x1000)
void *ksbrk(size_t s) {
  uintptr_t rc = (uintptr_t)align_page((void *)data_end);
  data_end += s;
  data_end = (uintptr_t)align_page((void *)data_end);

  if (!get_active_pagedirectory()) {
    // If there is no active pagedirectory we
    // just assume that the memory is
    // already mapped.
    return (void *)rc;
  }
  mmu_allocate_shared_kernel_region((void *)rc, (data_end - (uintptr_t)rc));
  assert(((uintptr_t)rc % PAGE_SIZE) == 0);
  memset((void *)rc, 0xFF, s);
  return (void *)rc;
}

void *ksbrk_physical(size_t s, void **physical) {
  void *r = ksbrk(s);
  if (physical) {
    //    if (0 == get_active_pagedirectory())
    //      *physical = (void *)((uintptr_t)r - (0xC0000000 + PHYS) + HEAP);
    //    else
    *physical = (void *)virtual_to_physical(r, 0);
  }
  return r;
}*/
