#include <assert.h>
#include <cpu/arch_inst.h>
#include <ksbrk.h>
#include <log.h>
#include <math.h>
#include <mmu.h>
#include <random.h>

#define INDEX_FROM_BIT(a) (a / (32))
#define OFFSET_FROM_BIT(a) (a % (32))

PageDirectory *kernel_directory;
PageDirectory real_kernel_directory;
PageDirectory *active_directory = 0;

u32 num_allocated_frames = 0;

#define END_OF_MEMORY 0x8000000 * 15
u64 available_memory_kb;

u32 num_array_frames = 1024;
u32 tmp_array[1024];
u32 *tmp_small_frames = tmp_array;

#define KERNEL_START 0xc0000000
extern uintptr_t data_end;

void change_frame(u32 frame, int on);
int get_free_frame(u32 *frame);
int allocate_frame(Page *page, int rw, int is_kernel);

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
  // Determine whether we are approaching a unallocated table
  int table_index = 1 + (rc / (1024 * 0x1000));
  if (!kernel_directory->tables[table_index]) {
    u32 physical;
    active_directory->tables[table_index] = (PageTable *)0xDEADBEEF;
    active_directory->tables[table_index] =
        (PageTable *)ksbrk_physical(sizeof(PageTable), (void **)&physical);
    memset(active_directory->tables[table_index], 0, sizeof(PageTable));
    active_directory->physical_tables[table_index] = (u32)physical | 0x3;

    kernel_directory->tables[table_index] =
        active_directory->tables[table_index];
    kernel_directory->physical_tables[table_index] =
        active_directory->physical_tables[table_index];
    return ksbrk(s);
  }
  if (!mmu_allocate_shared_kernel_region((void *)rc,
                                         (data_end - (uintptr_t)rc))) {
    return NULL;
  }
  assert(((uintptr_t)rc % PAGE_SIZE) == 0);
  return (void *)rc;
}

void *ksbrk_physical(size_t s, void **physical) {
  void *r = ksbrk(s);
  if (physical) {
    *physical = (void *)virtual_to_physical(r, 0);
  }
  return r;
}

u32 mmu_get_number_of_allocated_frames(void) {
  return num_allocated_frames;
}

Page *get_page(void *ptr, PageDirectory *directory, int create_new_page,
               int set_user) {
  uintptr_t address = (uintptr_t)ptr;
  if (!directory) {
    directory = get_active_pagedirectory();
  }
  address /= 0x1000;

  u32 table_index = address / 1024;
  if (!directory->tables[table_index]) {
    if (!create_new_page) {
      return 0;
    }

    u32 physical;
    directory->tables[table_index] =
        (PageTable *)kmalloc_align(sizeof(PageTable), (void **)&physical);
    memset(directory->tables[table_index], 0, sizeof(PageTable));
    directory->physical_tables[table_index] =
        (u32)physical | ((set_user) ? 0x7 : 0x3);

    if (!set_user) {
      kernel_directory->tables[table_index] = directory->tables[table_index];
      kernel_directory->physical_tables[table_index] =
          directory->physical_tables[table_index];
    }
  }
  Page *p = &directory->tables[table_index]->pages[address % 1024];
  if (create_new_page) {
    p->present = 0;
    p->user = set_user;
  }
  return &directory->tables[table_index]->pages[address % 1024];
}

void mmu_free_pages(void *a, u32 n) {
  for (; n > 0; n--) {
    Page *p = get_page(a, NULL, PAGE_NO_ALLOCATE, 0);
    p->present = 0;
    (void)write_to_frame(p->frame * 0x1000, 0);
    a += 0x1000;
  }
}

u32 start_frame_search = 1;
int get_free_frame(u32 *frame) {
  u32 i = start_frame_search;
  for (; i < INDEX_FROM_BIT(num_array_frames * 32); i++) {
    if (tmp_small_frames[i] == 0xFFFFFFFF) {
      continue;
    }

    for (u32 c = 0; c < 32; c++) {
      if (!(tmp_small_frames[i] & ((u32)1 << c))) {
        start_frame_search = i;
        *frame = i * 32 + c;
        return 1;
      }
    }
  }
  klog("MMU: Ran out of free frames. TODO: free up memory", LOG_WARN);
  *frame = 0;
  return 0;
}

int write_to_frame(u32 frame_address, u8 on) {
  u32 frame = frame_address / 0x1000;
  if (on) {
    int frame_is_used = (0 != (tmp_small_frames[INDEX_FROM_BIT(frame)] &
          ((u32)0x1 << OFFSET_FROM_BIT(frame))));
    if (frame_is_used) {
      return 0;
    }
    num_allocated_frames++;
    tmp_small_frames[INDEX_FROM_BIT(frame)] |=
        ((u32)0x1 << OFFSET_FROM_BIT(frame));
  } else {
    num_allocated_frames--;
    start_frame_search = min(start_frame_search, INDEX_FROM_BIT(frame));
    tmp_small_frames[INDEX_FROM_BIT(frame)] &=
        ~((u32)0x1 << OFFSET_FROM_BIT(frame));
  }
  return 1;
}

PageDirectory *get_active_pagedirectory(void) {
  return active_directory;
}

PageTable *clone_table(u32 src_index, PageDirectory *src_directory,
                       u32 *physical_address) {
  PageTable *new_table =
      kmalloc_align(sizeof(PageTable), (void **)physical_address);
  PageTable *src = src_directory->tables[src_index];

  // Copy all the pages
  for (u16 i = 0; i < 1024; i++) {
    if (!src->pages[i].present) {
      new_table->pages[i].present = 0;
      continue;
    }
    u32 frame_address;
    if (!get_free_frame(&frame_address)) {
      kmalloc_align_free(new_table, sizeof(PageTable));
      return NULL;
    }
    assert(write_to_frame(frame_address * 0x1000, 1));
    new_table->pages[i].frame = frame_address;

    new_table->pages[i].present |= src->pages[i].present;
    new_table->pages[i].rw |= src->pages[i].rw;
    new_table->pages[i].user |= src->pages[i].user;
    new_table->pages[i].accessed |= src->pages[i].accessed;
    new_table->pages[i].dirty |= src->pages[i].dirty;
  }

  // Now copy all of the data to the new table. This is done by creating a
  // virutal pointer to this newly created tables physical frame so we can
  // copy data to it.
  for (u32 i = 0; i < 1024; i++) {
    // Find a unused table
    if (src_directory->tables[i]) {
      continue;
    }

    // Link the table to the new table temporarily
    src_directory->tables[i] = new_table;
    src_directory->physical_tables[i] = *physical_address | 0x7;
    PageDirectory *tmp = get_active_pagedirectory();
    switch_page_directory(src_directory);

    // For each page in the table copy all the data over.
    for (u32 c = 0; c < 1024; c++) {
      // Only copy pages that are used.
      if (!src->pages[c].frame || !src->pages[c].present) {
        continue;
      }

      u32 table_data_pointer = i << 22 | c << 12;
      u32 src_data_pointer = src_index << 22 | c << 12;
      memcpy((void *)table_data_pointer, (void *)src_data_pointer, 0x1000);
    }
    src_directory->tables[i] = 0;
    src_directory->physical_tables[i] = 0;
    switch_page_directory(tmp);
    return new_table;
  }
  ASSERT_NOT_REACHED;
  return 0;
}

PageTable *copy_table(PageTable *src, u32 *physical_address) {
  PageTable *new_table =
      kmalloc_align(sizeof(PageTable), (void **)physical_address);

  // copy all the pages
  for (u16 i = 0; i < 1024; i++) {
    if (!src->pages[i].present) {
      new_table->pages[i].present = 0;
      continue;
    }
    new_table->pages[i].frame = src->pages[i].frame;
    new_table->pages[i].present = src->pages[i].present;
    new_table->pages[i].rw = src->pages[i].rw;
    new_table->pages[i].user = src->pages[i].user;
    new_table->pages[i].accessed = src->pages[i].accessed;
    new_table->pages[i].dirty = src->pages[i].dirty;
  }
  return new_table;
}

PageDirectory *clone_directory(PageDirectory *original) {
  if (!original) {
    original = get_active_pagedirectory();
  }

  u32 physical_address;
  PageDirectory *new_directory =
      kmalloc_align(sizeof(PageDirectory), (void **)&physical_address);
  u32 offset = (u32)new_directory->physical_tables - (u32)new_directory;
  new_directory->physical_address = physical_address + offset;

  for (int i = 0; i < 1024; i++) {
    if (!original->tables[i] && !kernel_directory->tables[i]) {
      new_directory->tables[i] = NULL;
      new_directory->physical_tables[i] = (u32)NULL;
      continue;
    }

    // Make sure to copy instead of cloning the stack.
    if (i >= 635 && i <= 641) {
      u32 physical;
      new_directory->tables[i] = clone_table(i, original, &physical);
      if (!new_directory->tables[i]) {
        mmu_free_pagedirectory(new_directory);
        return NULL;
      }
      new_directory->physical_tables[i] =
          physical | (original->physical_tables[i] & 0xFFF);
      continue;
    }

    if (original->tables[i] == kernel_directory->tables[i] || i > 641) {
      if (original->tables[i]) {
        assert(kernel_directory->tables[i]);
      }
      new_directory->tables[i] = kernel_directory->tables[i];
      new_directory->physical_tables[i] = kernel_directory->physical_tables[i];
      continue;
    }

    u32 physical;
    new_directory->tables[i] = clone_table(i, original, &physical);
    if (!new_directory->tables[i]) {
      mmu_free_pagedirectory(new_directory);
      return NULL;
    }
    new_directory->physical_tables[i] =
        physical | (original->physical_tables[i] & 0xFFF);
  }

  return new_directory;
}

int mmu_allocate_shared_kernel_region(void *rc, size_t n) {
  size_t num_pages = n / PAGE_SIZE;
  for (size_t i = 0; i <= num_pages; i++) {
    Page *p = get_page((void *)(rc + i * 0x1000), NULL, PAGE_ALLOCATE, 0);
    if (!p->present || !p->frame) {
      if (!allocate_frame(p, 0, 1)) {
        mmu_free_address_range(rc, n, NULL);
        return 0;
      }
    }
  }
  return 1;
}

void mmu_remove_virtual_physical_address_mapping(void *ptr, size_t length) {
  size_t num_pages = (uintptr_t)align_page((void *)length) / PAGE_SIZE;
  for (size_t i = 0; i < num_pages; i++) {
    Page *p = get_page(ptr + (i * PAGE_SIZE), NULL, PAGE_NO_ALLOCATE, 0);
    if (!p) {
      return;
    }
    p->frame = 0;
    p->present = 0;
  }
}

void *mmu_find_unallocated_virtual_range(void *addr, size_t length) {
  addr = align_page(addr);
  // Check if the pages already exist
  for (size_t i = 0; i < length; i += 0x1000) {
    if (get_page(addr + i, NULL, PAGE_NO_ALLOCATE, 0)) {
      // Page already exists
      return mmu_find_unallocated_virtual_range(addr + 0x1000, length);
    }
  }
  return addr;
}

int mmu_allocate_region(void *ptr, size_t n, mmu_flags flags,
                        PageDirectory *pd) {
  pd = (pd) ? pd : get_active_pagedirectory();
  size_t num_pages = n / 0x1000;
  for (size_t i = 0; i <= num_pages; i++) {
    Page *p = get_page((void *)(ptr + i * 0x1000), pd, 0, 0);
    if (p && p->present) {
      p->rw = (flags & MMU_FLAG_RW);
      p->user = !(flags & MMU_FLAG_KERNEL);
      continue;
    }
    p = get_page((void *)(ptr + i * 0x1000), pd, PAGE_ALLOCATE, 1);
    assert(p);
    int rw = (flags & MMU_FLAG_RW);
    int kernel = (flags & MMU_FLAG_KERNEL);
    if (!allocate_frame(p, rw, kernel)) {
      mmu_free_address_range(ptr, n, pd);
      return 0;
    }
  }
  flush_tlb();
  return 1;
}

void *mmu_map_user_frames(void *const ptr, size_t s) {
  void *const r = get_free_virtual_memory(s);
  size_t num_pages = s / 0x1000;
  for (size_t i = 0; i <= num_pages; i++) {
    Page *p = get_page((void *)(r + i * 0x1000), NULL, PAGE_ALLOCATE, 0);
    assert(p);
    int rw = 1;
    int is_kernel = 0;
    p->present = 1;
    p->rw = rw;
    p->user = !is_kernel;
    p->frame = (uintptr_t)(ptr + i * 0x1000) / 0x1000;
    kprintf("mapped user frame: %x\n", p->frame);
    (void)write_to_frame((uintptr_t)ptr + i * 0x1000, 1);
  }
  return r;
}

void *mmu_map_frames(void *const ptr, size_t s) {
  void *const r = mmu_find_unallocated_virtual_range((void *)0xEF000000, s);
  size_t num_pages = s / 0x1000;
  for (size_t i = 0; i <= num_pages; i++) {
    Page *p = get_page((void *)(r + i * 0x1000), NULL, PAGE_ALLOCATE, 0);
    assert(p);
    int rw = 1;
    int is_kernel = 1;
    p->present = 1;
    p->rw = rw;
    p->user = !is_kernel;
    p->frame = (uintptr_t)(ptr + i * 0x1000) / 0x1000;
    (void)write_to_frame((uintptr_t)ptr + i * 0x1000, 1);
  }
  return r;
}

int allocate_frame(Page *page, int rw, int is_kernel) {
  if (page->present) {
    klog("Page is already set", 1);
    assert(0);
    return 0;
  }
  u32 frame_address;
  if (!get_free_frame(&frame_address)) {
    return 0;
  }
  assert(write_to_frame(frame_address * 0x1000, 1));

  page->present = 1;
  page->rw = rw;
  page->user = !is_kernel;
  page->frame = frame_address;
  return 1;
}

void mmu_free_pagedirectory(PageDirectory *pd) {
  for (int i = 0; i < 1024; i++) {
    if (!pd->tables[i]) {
      continue;
    }
    if (pd->tables[i] == kernel_directory->tables[i]) {
      continue;
    }

    for (int j = 0; j < 1024; j++) {
      Page *page = &(pd->tables[i]->pages[j]);
      if (!page) {
        continue;
      }
      if (!page->present) {
        continue;
      }
      if (!page->frame) {
        continue;
      }
      (void)write_to_frame(((u32)page->frame) * 0x1000, 0);
    }
    kmalloc_align_free(pd->tables[i], sizeof(PageTable));
    pd->tables[i] = NULL;
  }
  kmalloc_align_free(pd, sizeof(PageDirectory));
}

void mmu_free_address_range(void *ptr, size_t length, PageDirectory *pd) {
  size_t num_pages = (size_t)align_page((void *)length) / PAGE_SIZE;
  for (size_t i = 0; i < num_pages; i++, ptr += PAGE_SIZE) {
    Page *page = get_page(ptr, pd, PAGE_NO_ALLOCATE, 0);
    if (!page) {
      continue;
    }
    if (!page->present) {
      continue;
    }
    if (!page->frame) {
      continue;
    }
    (void)write_to_frame(((u32)page->frame) * 0x1000, 0);
    page->present = 0;
    page->rw = 0;
    page->user = 0;
    page->frame = 0;
  }
}

void mmu_map_directories(void *dst, PageDirectory *d, void *src,
                         PageDirectory *s, size_t length) {
  d = (!d) ? get_active_pagedirectory() : d;
  s = (!s) ? get_active_pagedirectory() : s;
  size_t num_pages = (u32)align_page((void *)length) / 0x1000;
  for (size_t i = 0; i < num_pages; i++, dst += 0x1000, src += 0x1000) {
    Page *p = get_page(dst, d, PAGE_ALLOCATE, 1);
    p->present = 1;
    p->rw = 1;
    p->user = 1;
    void *physical = virtual_to_physical(src, s);
    p->frame = (u32)physical / PAGE_SIZE;
  }
  flush_tlb();
}

void mmu_map_physical(void *dst, PageDirectory *d, void *physical,
                      size_t length) {
  d = (!d) ? get_active_pagedirectory() : d;
  size_t num_pages = (u32)align_page((void *)length) / 0x1000;
  for (size_t i = 0; i < num_pages; i++, dst += 0x1000, physical += 0x1000) {
    Page *p = get_page(dst, d, PAGE_ALLOCATE, 1);
    p->present = 1;
    p->rw = 1;
    p->user = 1;
    p->frame = (uintptr_t)physical / PAGE_SIZE;
    (void)write_to_frame((uintptr_t)physical, 1);
  }
}

struct PhysVirtMap {
  u32 physical;
  u32 virtual;
  u32 length;
  u8 in_use;
};

struct PhysVirtMap phys_to_virt_map[256] = {0};

void create_physical_to_virtual_mapping(void *physical, void *virtual,
                                        u32 length) {
  for (u16 i = 0; i < 256; i++) {
    if (phys_to_virt_map[i].in_use) {
      continue;
    }
    phys_to_virt_map[i].physical = (u32)physical;
    phys_to_virt_map[i].virtual = (u32) virtual;
    phys_to_virt_map[i].length = length;
    phys_to_virt_map[i].in_use = 1;
    return;
  }
  assert(0);
}

void *physical_to_virtual(void *address) {
  for (u16 i = 0; i < 256; i++) {
    if (!phys_to_virt_map[i].in_use) {
      continue;
    }
    if (phys_to_virt_map[i].physical + phys_to_virt_map[i].length <
        (u32)address) {
      continue;
    }
    if (phys_to_virt_map[i].physical > (u32)address) {
      continue;
    }
    return (void *)phys_to_virt_map[i].virtual;
  }
  assert(0);
  return NULL;
}

void *virtual_to_physical(void *address, PageDirectory *directory) {
  if (0 == directory) {
    directory = get_active_pagedirectory();
  }
  Page *p = get_page((void *)address, directory, PAGE_NO_ALLOCATE, 0);
  return (void *)((u32)p->frame * 0x1000) + (((uintptr_t)address) & 0xFFF);
}

extern u32 inital_esp;
int move_stack(u32 new_stack_address, u32 size) {
  if (!mmu_allocate_region((void *)(new_stack_address - size), size,
                           MMU_FLAG_KERNEL, NULL)) {
    return 0;
  }

  u32 old_stack_pointer, old_base_pointer;

  old_stack_pointer = get_current_sp();
  old_base_pointer = get_current_sbp();

  u32 new_stack_pointer =
      old_stack_pointer + ((u32)new_stack_address - inital_esp);
  u32 new_base_pointer =
      old_base_pointer + ((u32)new_stack_address - inital_esp);

  // Copy the stack
  memcpy((void *)new_stack_pointer, (void *)old_stack_pointer,
         inital_esp - old_stack_pointer);
  for (u32 i = (u32)new_stack_address; i > new_stack_address - size; i -= 4) {
    u32 tmp = *(u32 *)i;
    if (old_stack_pointer < tmp && tmp < inital_esp) {
      tmp = tmp + (new_stack_address - inital_esp);
      u32 *tmp2 = (u32 *)i;
      *tmp2 = tmp;
    }
  }

  inital_esp = new_stack_pointer;
  // Actually change the stack
  set_sp(new_stack_pointer + 8);
  set_sbp(new_base_pointer);
  return 1;
}

// C strings have a unknown length so it does not makes sense to check
// for a size on the pointer. Instead we check whether the page it
// resides in is accessible to the user.
void *mmu_is_valid_user_c_string(const char *ptr, size_t *size) {
  void *r = (void *)ptr;
  size_t s = 0;
  for (; ((u32)ptr - (u32)r) < 0x1000;) {
    void *page = (void *)((uintptr_t)ptr & (uintptr_t)(~(PAGE_SIZE - 1)));
    if (!mmu_is_valid_userpointer(page, PAGE_SIZE)) {
      return NULL;
    }
    if (!((uintptr_t)ptr & (PAGE_SIZE - 1))) {
      ptr++;
      s++;
    }
    for (; (uintptr_t)ptr & (PAGE_SIZE - 1); ptr++, s++) {
      if (!*ptr) {
        if (size) {
          *size = s;
        }
        return r;
      }
    }
  }
  // String is too long, something has probably gone wrong.
  assert(0);
  return NULL;
}

void *mmu_is_valid_userpointer(const void *ptr, size_t s) {
  uintptr_t t = (uintptr_t)ptr;
  size_t num_pages = (uintptr_t)align_page((void *)s) / 0x1000;
  for (size_t i = 0; i < num_pages; i++, t += 0x1000) {
    Page *page = get_page((void *)t, NULL, PAGE_NO_ALLOCATE, 0);
    if (!page) {
      return NULL;
    }
    if (!page->present) {
      return NULL;
    }
    if (!page->user) {
      return NULL;
    }
  }
  return (void *)ptr;
}

void switch_page_directory(PageDirectory *directory) {
  active_directory = directory;
  set_cr3(directory->physical_address);
}

void create_table(int table_index) {
  if (kernel_directory->tables[table_index]) {
    return;
  }
  u32 physical;
  kernel_directory->tables[table_index] = (PageTable *)0xDEADBEEF;
  kernel_directory->tables[table_index] =
      (PageTable *)kmalloc_align(sizeof(PageTable), (void **)&physical);
  memset(kernel_directory->tables[table_index], 0, sizeof(PageTable));
  kernel_directory->physical_tables[table_index] = (u32)physical | 0x3;
}

void paging_init(u64 memsize, multiboot_info_t *mb) {
  u32 *cr3 = (void *)get_cr3();
  u32 *virtual = (u32 *)((u32)cr3 + 0xC0000000);

  u32 num_of_frames = 0;

  memset(tmp_small_frames, 0xFF, num_array_frames * sizeof(u32));
  {
    multiboot_memory_map_t *map =
        (multiboot_memory_map_t *)(mb->mmap_addr + 0xc0000000);
    for (size_t length = 0; length < mb->mmap_length;) {
      if (MULTIBOOT_MEMORY_AVAILABLE == map->type) {
        num_of_frames = max(num_of_frames, map->addr + map->len + 0x1000);
        for (size_t i = 0; i < map->len; i += 0x20000) {
          u32 frame = (map->addr + i) / 0x1000;
          if (frame < (num_array_frames * 32)) {
            tmp_small_frames[INDEX_FROM_BIT(frame)] = 0;
          }
        }
      }
      u32 delta = (uintptr_t)map->size + sizeof(map->size);
      map = (multiboot_memory_map_t *)((uintptr_t)map + delta);
      length += delta;
    }
  }
  num_of_frames /= 0x1000;
  num_of_frames /= 32;

  kernel_directory = &real_kernel_directory;
  kernel_directory->physical_address = (u32)cr3;
  for (u32 i = 0; i < 1024; i++) {
    kernel_directory->physical_tables[i] = virtual[i];

    if (!kernel_directory->physical_tables[i]) {
      kernel_directory->tables[i] = NULL;
      continue;
    }

    kernel_directory->tables[i] =
        (PageTable *)(0xC0000000 + (virtual[i] & ~(0xFFF)));

    // Loop through the pages in the table
    PageTable *table = kernel_directory->tables[i];
    (void)write_to_frame(kernel_directory->physical_tables[i], 1);
    for (size_t j = 0; j < 1024; j++) {
      if (!table->pages[j].present) {
        continue;
      }
      // Add the frame to our bitmap to ensure it does not get used by
      // another newly created page.
      (void)write_to_frame(table->pages[j].frame * 0x1000, 1);
    }
  }

  switch_page_directory(kernel_directory);
  // Make null dereferences crash.
  get_page(NULL, kernel_directory, PAGE_ALLOCATE, 0)->present = 0;
  for (int i = 0; i < 25; i++) {
    create_table(770 + i);
  }
  kernel_directory = clone_directory(kernel_directory);
  assert(kernel_directory);

  switch_page_directory(kernel_directory);
  {
    PageDirectory *tmp = clone_directory(kernel_directory);
    assert(tmp);
    switch_page_directory(tmp);
  }
  assert(move_stack(0xA0000000, 0x80000));

  available_memory_kb = memsize;

  num_of_frames = max(num_of_frames, num_array_frames);
  void *new = kmalloc(num_of_frames * sizeof(u32));
  memset(new, 0xFF, num_of_frames * sizeof(u32));
  memcpy(new, tmp_small_frames, num_array_frames * sizeof(u32));
  tmp_small_frames = new;
  {
    multiboot_memory_map_t *map =
        (multiboot_memory_map_t *)(mb->mmap_addr + 0xc0000000);
    for (size_t length = 0; length < mb->mmap_length;) {
      if (MULTIBOOT_MEMORY_AVAILABLE == map->type) {
        for (size_t i = 0; i < map->len - 0x1000; i += 0x20000) {
          u32 frame = (map->addr + i) / 0x1000;
          if (frame > (num_array_frames * 32)) {
            assert(INDEX_FROM_BIT(frame) < num_of_frames);
            tmp_small_frames[INDEX_FROM_BIT(frame)] = 0;
          }
        }
      }
      u32 delta = (uintptr_t)map->size + sizeof(map->size);
      map = (multiboot_memory_map_t *)((uintptr_t)map + delta);
      length += delta;
    }
  }
  num_array_frames = num_of_frames;
}
