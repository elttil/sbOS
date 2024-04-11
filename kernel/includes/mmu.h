#ifndef PAGING_H
#define PAGING_H
#include "kmalloc.h"
#include <multiboot.h>
#include <typedefs.h>

typedef u8 mmu_flags;

#define MMU_FLAG_RW (1 << 0)
#define MMU_FLAG_KERNEL (1 << 1)

#define PAGE_SIZE ((uintptr_t)0x1000)
#define next_page(_ptr)                                                        \
  ((_ptr) + (PAGE_SIZE - (((uintptr_t)_ptr) & (PAGE_SIZE - 1))))
#define align_page(_ptr)                                                       \
  (((((uintptr_t)_ptr) & (PAGE_SIZE - 1)) > 0) ? next_page((_ptr)) : (_ptr))

typedef struct Page {
  u32 present : 1;
  u32 rw : 1;
  u32 user : 1;
  u32 accessed : 1;
  u32 dirty : 1;
  u32 unused : 7;
  u32 frame : 20;
} __attribute__((packed)) Page;

typedef struct PageTable {
  Page pages[1024];
} __attribute__((packed)) PageTable;

typedef struct PageDirectory {
  PageTable *tables[1024];
  u32 physical_tables[1024];
  u32 physical_address;
} PageDirectory;

int mmu_allocate_region(void *ptr, size_t n, mmu_flags flags,
                        PageDirectory *pd);
void mmu_allocate_shared_kernel_region(void *rc, size_t n);
void *mmu_find_unallocated_virtual_range(void *addr, size_t length);
void mmu_remove_virtual_physical_address_mapping(void *ptr, size_t length);
void mmu_free_address_range(void *ptr, size_t length, PageDirectory *pd);
void mmu_map_directories(void *dst, PageDirectory *d, void *src,
                         PageDirectory *s, size_t length);
u32 mmu_get_number_of_allocated_frames(void);
void *mmu_map_frames(void *ptr, size_t s);
void mmu_map_physical(void *dst, PageDirectory *d, void *physical,
                      size_t length);
void mmu_free_pages(void *a, u32 n);
void *mmu_map_user_frames(void *const ptr, size_t s);
void *mmu_is_valid_userpointer(const void *ptr, size_t s);
void *mmu_is_valid_user_c_string(const char *ptr, size_t *size);

void flush_tlb(void);
void paging_init(u64 memsize, multiboot_info_t *mb);
PageDirectory *get_active_pagedirectory(void);
void move_stack(u32 new_stack_address, u32 size);
void switch_page_directory(PageDirectory *directory);
void *allocate_frame(Page *page, int rw, int is_kernel);
PageDirectory *clone_directory(PageDirectory *original);
void *virtual_to_physical(void *address, PageDirectory *directory);
void *ksbrk(size_t s);
void *ksbrk_physical(size_t s, void **physical);
void write_to_frame(u32 frame_address, u8 on);

Page *get_page(void *ptr, PageDirectory *directory, int create_new_page,
               int set_user);

void create_physical_to_virtual_mapping(void *physical, void *virtual,
                                        u32 length);
// This can only be used on addreses that be been mapped using:
// create_physical_to_virtual_mapping
void *physical_to_virtual(void *address);
#endif
