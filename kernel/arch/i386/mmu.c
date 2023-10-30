#include <assert.h>
#include <ksbrk.h>
#include <log.h>
#include <mmu.h>

#define INDEX_FROM_BIT(a) (a / (32))
#define OFFSET_FROM_BIT(a) (a % (32))
#define PAGE_SIZE ((uintptr_t)0x1000)

#define PAGE_ALLOCATE 1
#define PAGE_NO_ALLOCATE 0

PageDirectory *kernel_directory;
PageDirectory real_kernel_directory;
PageDirectory *active_directory = 0;

#define END_OF_MEMORY 0x8000000 * 15
const uint32_t num_of_frames = END_OF_MEMORY / PAGE_SIZE;
uint32_t frames[END_OF_MEMORY / PAGE_SIZE] = {0};
uint32_t num_allocated_frames = 0;

#define KERNEL_START 0xc0000000
extern uintptr_t data_end;

void write_to_frame(uint32_t frame_address, uint8_t on);

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
  if (!active_directory->tables[table_index]) {
    uint32_t physical;
    active_directory->tables[table_index] = (PageTable *)0xDEADBEEF;
    active_directory->tables[table_index] =
        (PageTable *)ksbrk_physical(sizeof(PageTable), (void **)&physical);
    memset(active_directory->tables[table_index], 0, sizeof(PageTable));
    active_directory->physical_tables[table_index] = (uint32_t)physical | 0x3;

    kernel_directory->tables[table_index] =
        active_directory->tables[table_index];
    kernel_directory->physical_tables[table_index] =
        active_directory->physical_tables[table_index];
    return ksbrk(s);
  }
  mmu_allocate_shared_kernel_region((void *)rc, (data_end - (uintptr_t)rc));
  assert(((uintptr_t)rc % PAGE_SIZE) == 0);
  memset((void *)rc, 0x00, s);

  return (void *)rc;
}

void *ksbrk_physical(size_t s, void **physical) {
  void *r = ksbrk(s);
  if (physical) {
    *physical = (void *)virtual_to_physical(r, 0);
  }
  return r;
}

uint32_t mmu_get_number_of_allocated_frames(void) {
  return num_allocated_frames;
}

Page *get_page(void *ptr, PageDirectory *directory, int create_new_page,
               int set_user) {
  uintptr_t address = (uintptr_t)ptr;
  if (!directory)
    directory = get_active_pagedirectory();
  address /= 0x1000;

  uint32_t table_index = address / 1024;
  if (!directory->tables[table_index]) {
    if (!create_new_page)
      return 0;

    uint32_t physical;
    directory->tables[table_index] =
        (PageTable *)ksbrk_physical(sizeof(PageTable), (void **)&physical);
    memset(directory->tables[table_index], 0, sizeof(PageTable));
    directory->physical_tables[table_index] =
        (uint32_t)physical | ((set_user) ? 0x7 : 0x3);

    if (!set_user) {
      kernel_directory->tables[table_index] = directory->tables[table_index];
      kernel_directory->physical_tables[table_index] =
          directory->physical_tables[table_index];
    }
  }
  Page *p = &directory->tables[table_index]->pages[address % 1024];
  if (create_new_page) {
    p->present = 0;
  }
  return &directory->tables[table_index]->pages[address % 1024];
}

void mmu_free_pages(void *a, uint32_t n) {
  for (; n > 0; n--) {
    Page *p = get_page(a, NULL, PAGE_NO_ALLOCATE, 0);
    p->present = 0;
    write_to_frame(p->frame * 0x1000, 0);
    a += 0x1000;
  }
}

void *next_page(void *ptr) {
  uintptr_t a = (uintptr_t)ptr;
  return (void *)(a + (PAGE_SIZE - ((uint32_t)a & (PAGE_SIZE - 1))));
}

void *align_page(void *a) {
  if ((uintptr_t)a & (PAGE_SIZE - 1))
    return next_page(a);

  return a;
}

void flush_tlb(void) {
  asm volatile("\
	mov %cr3, %eax;\
	mov %eax, %cr3");
}

uint32_t first_free_frame(void) {
  for (uint32_t i = 1; i < INDEX_FROM_BIT(num_of_frames); i++) {
    if (frames[i] == 0xFFFFFFFF)
      continue;

    for (uint32_t c = 0; c < 32; c++)
      if (!(frames[i] & ((uint32_t)1 << c)))
        return i * 32 + c;
  }

  kprintf("ERROR Num frames: %x\n", mmu_get_number_of_allocated_frames());
  klog("No free frames, uh oh.", LOG_ERROR);
  assert(0);
  return 0;
}

void write_to_frame(uint32_t frame_address, uint8_t on) {
  uint32_t frame = frame_address / 0x1000;
  assert(INDEX_FROM_BIT(frame) < sizeof(frames) / sizeof(frames[0]));
  if (on) {
    num_allocated_frames++;
    frames[INDEX_FROM_BIT(frame)] |= ((uint32_t)0x1 << OFFSET_FROM_BIT(frame));
    return;
  }
  num_allocated_frames--;
  frames[INDEX_FROM_BIT(frame)] &= ~((uint32_t)0x1 << OFFSET_FROM_BIT(frame));
}

PageDirectory *get_active_pagedirectory(void) { return active_directory; }

PageTable *clone_table(uint32_t src_index, PageDirectory *src_directory,
                       uint32_t *physical_address) {
  PageTable *new_table =
      ksbrk_physical(sizeof(PageTable), (void **)physical_address);
  PageTable *src = src_directory->tables[src_index];

  // Copy all the pages
  for (uint16_t i = 0; i < 1024; i++) {
    if (!src->pages[i].present) {
      new_table->pages[i].present = 0;
      continue;
    }
    uint32_t frame_address = first_free_frame();
    write_to_frame(frame_address * 0x1000, 1);
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
  for (uint32_t i = 0; i < 1024; i++) {
    // Find a unused table
    if (src_directory->tables[i])
      continue;

    // Link the table to the new table temporarily
    src_directory->tables[i] = new_table;
    src_directory->physical_tables[i] = *physical_address | 0x7;
    PageDirectory *tmp = get_active_pagedirectory();
    switch_page_directory(src_directory);

    // For each page in the table copy all the data over.
    for (uint32_t c = 0; c < 1024; c++) {
      // Only copy pages that are used.
      if (!src->pages[c].frame || !src->pages[c].present)
        continue;

      uint32_t table_data_pointer = i << 22 | c << 12;
      uint32_t src_data_pointer = src_index << 22 | c << 12;
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

PageTable *copy_table(PageTable *src, uint32_t *physical_address) {
  PageTable *new_table =
      ksbrk_physical(sizeof(PageTable), (void **)physical_address);

  // copy all the pages
  for (uint16_t i = 0; i < 1024; i++) {
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
  if (!original)
    original = get_active_pagedirectory();

  uint32_t physical_address;
  PageDirectory *new_directory =
      ksbrk_physical(sizeof(PageDirectory), (void **)&physical_address);
  uint32_t offset =
      (uint32_t)new_directory->physical_tables - (uint32_t)new_directory;
  new_directory->physical_address = physical_address + offset;

  for (int i = 0; i < 1024; i++) {
    if (!original->tables[i]) {
      new_directory->tables[i] = NULL;
      new_directory->physical_tables[i] = (uint32_t)NULL;
      continue;
    }

    // Make sure to copy instead of cloning the stack.
    if (i >= 635 && i <= 641) {
      uint32_t physical;
      new_directory->tables[i] = clone_table(i, original, &physical);
      new_directory->physical_tables[i] =
          physical | (original->physical_tables[i] & 0xFFF);
      continue;
    }

    //    if (original->tables[i] == kernel_directory->tables[i]) {
    if (i > 641) {
      new_directory->tables[i] = kernel_directory->tables[i];
      new_directory->physical_tables[i] = kernel_directory->physical_tables[i];
      continue;
    }

    uint32_t physical;
    new_directory->tables[i] = clone_table(i, original, &physical);
    new_directory->physical_tables[i] =
        physical | (original->physical_tables[i] & 0xFFF);
  }

  return new_directory;
}

void mmu_allocate_shared_kernel_region(void *rc, size_t n) {
  size_t num_pages = n / PAGE_SIZE;
  for (size_t i = 0; i <= num_pages; i++) {
    Page *p = get_page((void *)(rc + i * 0x1000), NULL, PAGE_NO_ALLOCATE, 0);
    if (!p) {
      kprintf("don't have: %x\n", rc + i * 0x1000);
      p = get_page((void *)(rc + i * 0x1000), NULL, PAGE_ALLOCATE, 0);
    }
    if (!p->present || !p->frame)
      allocate_frame(p, 0, 1);
  }
}

void mmu_remove_virtual_physical_address_mapping(void *ptr, size_t length) {
  size_t num_pages = (uintptr_t)align_page((void *)length) / PAGE_SIZE;
  for (size_t i = 0; i < num_pages; i++) {
    Page *p = get_page(ptr + (i * PAGE_SIZE), NULL, PAGE_NO_ALLOCATE, 0);
    if (!p)
      return;
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
    Page *p = get_page((void *)(ptr + i * 0x1000), pd, PAGE_ALLOCATE, 1);
    assert(p);
    int rw = (flags & MMU_FLAG_RW);
    int kernel = (flags & MMU_FLAG_KERNEL);
    if (!allocate_frame(p, rw, kernel)) {
      klog("MMU: Frame allocation failed", LOG_WARN);
      return 0;
    }
  }
  return 1;
}

void *mmu_map_frames(void *const ptr, size_t s) {
  void *const r = mmu_find_unallocated_virtual_range((void *)0xEF000000, s);
  kprintf("r: %x\n", r);
  size_t num_pages = s / 0x1000;
  for (size_t i = 0; i <= num_pages; i++) {
    Page *p = get_page((void *)(r + i * 0x1000), NULL, PAGE_ALLOCATE, 1);
    assert(p);
    int rw = 1;
    int is_kernel = 1;
    p->present = 1;
    p->rw = rw;
    p->user = !is_kernel;
    p->frame = (uintptr_t)(ptr + i * 0x1000) / 0x1000;
    write_to_frame((uintptr_t)ptr + i * 0x1000, 1);
  }
  flush_tlb();
  return r;
}

void *allocate_frame(Page *page, int rw, int is_kernel) {
  if (page->present) {
    dump_backtrace(5);
    klog("Page is already set", 1);
    for (;;)
      ;
    return 0;
  }
  uint32_t frame_address = first_free_frame();
  assert(frame_address < sizeof(frames) / sizeof(frames[0]));
  write_to_frame(frame_address * 0x1000, 1);

  page->present = 1;
  page->rw = rw;
  page->user = !is_kernel;
  page->frame = frame_address;
  return (void *)(frame_address * 0x1000);
}

void mmu_free_address_range(void *ptr, size_t length) {
  size_t num_pages = (size_t)align_page((void *)length) / PAGE_SIZE;
  for (size_t i = 0; i < num_pages; i++, ptr += PAGE_SIZE) {
    Page *page = get_page(ptr, NULL, PAGE_NO_ALLOCATE, 0);
    if (!page)
      continue;
    if (!page->present)
      continue;
    if (!page->frame)
      continue;
    // Sanity check that none of the frames are invalid
    assert(page->frame < sizeof(frames) / sizeof(frames[0]));
    write_to_frame(((uint32_t)page->frame) * 0x1000, 0);
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
  size_t num_pages = (uint32_t)align_page((void *)length) / 0x1000;
  for (size_t i = 0; i < num_pages; i++, dst += 0x1000, src += 0x1000) {
    Page *p = get_page(dst, d, PAGE_ALLOCATE, 1);
    p->present = 1;
    p->rw = 1;
    p->user = 1;
    void *physical = virtual_to_physical(src, s);
    p->frame = (uint32_t)physical / PAGE_SIZE;
  }
  flush_tlb();
}

void mmu_map_physical(void *dst, PageDirectory *d, void *physical,
                      size_t length) {
  d = (!d) ? get_active_pagedirectory() : d;
  size_t num_pages = (uint32_t)align_page((void *)length) / 0x1000;
  for (size_t i = 0; i < num_pages; i++, dst += 0x1000, physical += 0x1000) {
    Page *p = get_page(dst, d, PAGE_ALLOCATE, 1);
    p->present = 1;
    p->rw = 1;
    p->user = 1;
    p->frame = (uintptr_t)physical / PAGE_SIZE;
    write_to_frame((uintptr_t)physical, 1);
  }
  flush_tlb();
}

void *virtual_to_physical(void *address, PageDirectory *directory) {
  if (0 == directory)
    directory = get_active_pagedirectory();
  return (void *)((get_page((void *)address, directory, PAGE_NO_ALLOCATE, 0)
                       ->frame *
                   0x1000) +
                  (((uintptr_t)address) & 0xFFF));
}

extern uint32_t inital_esp;
void __attribute__((optimize("O0")))
move_stack(uint32_t new_stack_address, uint32_t size) {
  mmu_allocate_region((void *)(new_stack_address - size), size,
                      MMU_FLAG_KERNEL, NULL);

  uint32_t old_stack_pointer, old_base_pointer;

  register uint32_t eax asm("eax");
  asm volatile("mov %esp, %eax");
  old_stack_pointer = eax;
  asm volatile("mov %ebp, %eax");
  old_base_pointer = eax;

  uint32_t new_stack_pointer =
      old_stack_pointer + ((uint32_t)new_stack_address - inital_esp);
  uint32_t new_base_pointer =
      old_base_pointer + ((uint32_t)new_stack_address - inital_esp);

  // Copy the stack
  memcpy((void *)new_stack_pointer, (void *)old_stack_pointer,
         inital_esp - old_stack_pointer);
  for (uint32_t i = (uint32_t)new_stack_address; i > new_stack_address - size;
       i -= 4) {
    uint32_t tmp = *(uint32_t *)i;
    if (old_stack_pointer < tmp && tmp < inital_esp) {
      tmp = tmp + (new_stack_address - inital_esp);
      uint32_t *tmp2 = (uint32_t *)i;
      *tmp2 = tmp;
    }
  }

  inital_esp = new_stack_pointer;
  // Actually change the stack
  eax = new_stack_pointer;
  asm volatile("mov %eax, %esp");
  eax = new_base_pointer;
  asm volatile("mov %eax, %ebp");
}

// C strings have a unknown length so it does not makes sense to check
// for a size on the pointer. Instead we check whether the page it
// resides in is accessible to the user.
void *is_valid_user_c_string(const char *ptr, size_t *size) {
  void *r = (void *)ptr;
  size_t s = 0;
  for (;;) {
    void *page = (void *)((uintptr_t)ptr & (uintptr_t)(~(PAGE_SIZE - 1)));
    if (!is_valid_userpointer(page, PAGE_SIZE))
      return NULL;
    for (; (uintptr_t)ptr & (PAGE_SIZE - 1); ptr++, s++)
      if (!*ptr) {
        if (size)
          *size = s;
        return r;
      }
  }
}

void *is_valid_userpointer(const void *ptr, size_t s) {
  uintptr_t t = (uintptr_t)ptr;
  size_t num_pages = (uintptr_t)align_page((void *)s) / 0x1000;
  for (size_t i = 0; i < num_pages; i++, t += 0x1000) {
    Page *page = get_page((void *)t, NULL, PAGE_NO_ALLOCATE, 0);
    if (!page)
      return NULL;
    if (!page->present)
      return NULL;
    if (!page->user)
      return NULL;
  }
  return (void *)ptr;
}

void switch_page_directory(PageDirectory *directory) {
  active_directory = directory;
  asm("mov %0, %%cr3" ::"r"(directory->physical_address));
}

void enable_paging(void) {
  asm("mov %cr0, %eax\n"
      "or 0b10000000000000000000000000000000, %eax\n"
      "mov %eax, %cr0\n");
}

extern uint32_t boot_page_directory;
extern uint32_t boot_page_table1;

void paging_init(void) {
  uint32_t *cr3;
  asm volatile("mov %%cr3, %0" : "=r"(cr3));
  uint32_t *virtual = (uint32_t *)((uint32_t)cr3 + 0xC0000000);

  kernel_directory = &real_kernel_directory;
  kernel_directory->physical_address = (uint32_t)cr3;
  for (uint32_t i = 0; i < 1024; i++) {
    kernel_directory->physical_tables[i] = virtual[i];

    if (!kernel_directory->physical_tables[i]) {
      kernel_directory->tables[i] = NULL;
      continue;
    }

    kernel_directory->tables[i] =
        (PageTable *)(0xC0000000 + (virtual[i] & ~(0xFFF)));

    // Loop through the pages in the table
    PageTable *table = kernel_directory->tables[i];
    for (size_t j = 0; j < 1024; j++) {
      if (!table->pages[j].present)
        continue;
      // Add the frame to our bitmap to ensure it does not get used by
      // another newly created page.
      write_to_frame(table->pages[j].frame * 0x1000, 1);
    }
  }

  switch_page_directory(kernel_directory);
  kernel_directory = clone_directory(kernel_directory);

  // Make null dereferences crash.
  switch_page_directory(kernel_directory);
  get_page(NULL, kernel_directory, PAGE_ALLOCATE, 0)->present = 0;
  switch_page_directory(clone_directory(kernel_directory));
  // FIXME: Really hacky solution. Since page table creation needs to
  // allocate memory but memory allocation requires page table creation
  // they depend on eachother. The bad/current solution is just to
  // allocate a bunch of tables so the memory allocator has enough.
  for (uintptr_t i = 0; i < 140; i++)
    allocate_frame(
        get_page((void *)((0x302 + i) * 0x1000 * 1024), NULL, PAGE_ALLOCATE, 0),
        1, 1);
  move_stack(0xA0000000, 0xC00000);
}
