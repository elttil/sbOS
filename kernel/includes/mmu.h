#ifndef PAGING_H
#define PAGING_H
#include "kmalloc.h"
#include <stdint.h>

typedef uint8_t mmu_flags;

#define MMU_FLAG_RW (1 << 0)
#define MMU_FLAG_KERNEL (1 << 1)

void *next_page(void *a);
void *align_page(void *a);

typedef struct Page {
  uint32_t present : 1;
  uint32_t rw : 1;
  uint32_t user : 1;
  uint32_t accessed : 1;
  uint32_t dirty : 1;
  uint32_t unused : 7;
  uint32_t frame : 20;
} __attribute__((packed)) Page;

typedef struct PageTable {
  Page pages[1024];
} __attribute__((packed)) PageTable;

typedef struct PageDirectory {
  PageTable *tables[1024];
  uint32_t physical_tables[1024];
  uint32_t physical_address;
} PageDirectory;

int mmu_allocate_region(void *ptr, size_t n, mmu_flags flags,
                        PageDirectory *pd);
void mmu_allocate_shared_kernel_region(void *rc, size_t n);
void *mmu_find_unallocated_virtual_range(void *addr, size_t length);
void mmu_remove_virtual_physical_address_mapping(void *ptr, size_t length);
void mmu_free_address_range(void *ptr, size_t length);
void mmu_map_directories(void *dst, PageDirectory *d, void *src,
                         PageDirectory *s, size_t length);
uint32_t mmu_get_number_of_allocated_frames(void);
void *mmu_map_frames(void *ptr, size_t s);
void mmu_map_physical(void *dst, PageDirectory *d, void *physical,
                      size_t length);
void mmu_free_pages(void *a, uint32_t n);

void flush_tlb(void);
void paging_init(uint64_t memsize);
PageDirectory *get_active_pagedirectory(void);
void move_stack(uint32_t new_stack_address, uint32_t size);
void switch_page_directory(PageDirectory *directory);
void *allocate_frame(Page *page, int rw, int is_kernel);
PageDirectory *clone_directory(PageDirectory *original);
void *virtual_to_physical(void *address, PageDirectory *directory);
void *is_valid_userpointer(const void *const p, size_t s);
void *is_valid_user_c_string(const char *ptr, size_t *size);
void *ksbrk(size_t s);

Page *get_page(void *ptr, PageDirectory *directory, int create_new_page,
               int set_user);
#endif
