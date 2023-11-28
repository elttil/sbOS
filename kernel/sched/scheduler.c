#include <assert.h>
#include <cpu/io.h>
#include <defs.h>
#include <drivers/pit.h>
#include <elf.h>
#include <errno.h>
#include <fs/vfs.h>

#define STACK_LOCATION ((void *)0x90000000)
#define STACK_SIZE 0x80000
#define HEAP_SIZE 0x400000
#define HEAP_LOCATION (STACK_LOCATION - STACK_SIZE - HEAP_SIZE)

process_t *ready_queue;
process_t *current_task = NULL;
u32 next_pid = 0;

extern u32 read_eip(void);

process_t *get_current_task(void) {
  return current_task;
}

bool get_task_from_pid(u32 pid, process_t **out) {
  for (process_t *tmp = ready_queue; tmp; tmp = tmp->next) {
    if (tmp->pid == pid) {
      *out = tmp;
      return true;
    }
  }
  return false;
}

void set_signal_handler(int sig, void (*handler)(int)) {
  if (sig >= 20 || sig < 0)
    return;
  if (9 == sig)
    return;
  current_task->signal_handlers[sig] = handler;
}

process_t *create_process(process_t *p) {
  process_t *r;
  r = ksbrk(sizeof(process_t));
  r->dead = 0;
  r->pid = next_pid++;
  r->esp = r->ebp = 0;
  r->eip = 0;
  r->sleep_until = 0;
  if (!p) {
    assert(1 == next_pid);
    strncpy(r->program_name, "[kernel]", sizeof(current_task->program_name));
  } else
    strncpy(r->program_name, "[Not yet named]",
            sizeof(current_task->program_name));

  r->cr3 = (p) ? clone_directory(get_active_pagedirectory())
               : get_active_pagedirectory();
  r->next = 0;
  r->incoming_signal = 0;
  r->parent = p;
  r->child = NULL;
  r->halt_list = NULL;

  mmu_allocate_region((void *)(0x80000000 - 0x1000), 0x1000, MMU_FLAG_RW,
                      r->cr3);
  r->signal_handler_stack = 0x80000000;

  if (p) {
    strcpy(r->current_working_directory, p->current_working_directory);
  } else {
    strcpy(r->current_working_directory, "/");
  }
  r->data_segment_end = (p) ? p->data_segment_end : NULL;
  memset((void *)r->halts, 0, 2 * sizeof(u32));
  for (int i = 0; i < 100; i++) {
    if (p) {
      r->file_descriptors[i] = p->file_descriptors[i];
      if (r->file_descriptors[i]) {
        r->file_descriptors[i]->reference_count++;
      }
    } else {
      r->file_descriptors[i] = NULL;
    }
    if (i < 20)
      r->signal_handlers[i] = NULL;
    r->read_halt_inode[i] = NULL;
    r->write_halt_inode[i] = NULL;
    r->disconnect_halt_inode[i] = NULL;
    r->maps[i] = NULL;
  }
  return r;
}

int get_free_fd(process_t *p, int allocate) {
  if (!p)
    p = (process_t *)current_task;
  int i;
  for (i = 0; i < 100; i++)
    if (!p->file_descriptors[i])
      break;
  if (p->file_descriptors[i])
    return -1;
  if (allocate) {
    vfs_fd_t *fd = p->file_descriptors[i] = kmalloc(sizeof(vfs_fd_t));
    fd->inode = kmalloc(sizeof(vfs_inode_t));
  }
  return i;
}

void tasking_init(void) {
  current_task = ready_queue = create_process(NULL);
}

int i = 0;
void free_process(void) {
  kprintf("Exiting process: %s\n", get_current_task()->program_name);
  // free_process() will purge all contents such as allocated frames
  // out of the current process. This will be called by exit() and
  // exec*().

  // Do a special free for shared memory which avoids labeling
  // underlying frames as "unused".
  for (int i = 0; i < 100; i++) {
    vfs_close(i);
    if (!current_task->maps[i])
      continue;
    MemoryMap *m = current_task->maps[i];
    mmu_remove_virtual_physical_address_mapping(m->u_address, m->length);
  }

  // NOTE: Kernel stuff begins at 0x90000000
  mmu_free_address_range((void *)0x1000, 0x90000000);
}

void exit(int status) {
  assert(current_task->pid != 1);
  if (current_task->parent) {
    current_task->parent->halts[WAIT_CHILD_HALT] = 0;
    current_task->parent->child_rc = status;
  }
  process_t *new_task = current_task;
  for (; new_task == current_task;) {
    if (!new_task->next)
      new_task = ready_queue;
    new_task = new_task->next;
  }

  free_process();
  // Remove current_task from list
  for (process_t *tmp = ready_queue; tmp;) {
    if (tmp == current_task) // current_task is ready_queue(TODO:
                             // Figure out whether this could even
                             // happen)
    {
      ready_queue = current_task->next;
      break;
    }
    if (tmp->next == current_task) {
      tmp->next = tmp->next->next;
      break;
    }
    tmp = tmp->next;
  }
  current_task->dead = 1;
  // This function will enable interrupts
  switch_task();
}

u32 setup_stack(u32 stack_pointer, int argc, char **argv) {
  mmu_allocate_region(STACK_LOCATION - STACK_SIZE, STACK_SIZE, MMU_FLAG_RW,
                      NULL);
  flush_tlb();

  u32 ptr = stack_pointer;

  char *argv_ptrs[argc + 1];
  for (int i = 0; i < argc; i++) {
    char *s = argv[i];
    size_t l = strlen(s);
    ptr -= l + 1;
    char *b = (char *)ptr;
    memcpy(b, s, l);
    b[l] = '\0';
    argv_ptrs[i] = b;
  }

  char **ptrs[argc + 1];
  for (int i = argc; i >= 0; i--) {
    ptr -= sizeof(char *);
    ptrs[i] = (char **)ptr;
    if (i != argc) {
      *(ptrs[i]) = argv_ptrs[i];
    } else {
      *(ptrs[i]) = NULL;
    }
  }

  char *s = (char *)ptr;
  ptr -= sizeof(char **);
  *(char ***)ptr = (char **)s;

  ptr -= sizeof(int);
  *(int *)ptr = argc;
  return ptr;
}

int exec(const char *filename, char **argv) {
  // exec() will "takeover" the process by loading the file specified in
  // filename into memory, change from ring 0 to ring 3 and jump to the
  // files entry point as decided by the ELF header of the file.
  int argc = 0;
  for (; argv[argc];) {
    argc++;
  }

  u32 end_of_code;
  void *entry = load_elf_file(filename, &end_of_code);
  if (!entry) {
    return 0;
  }

  strncpy(current_task->program_name, filename,
          sizeof(current_task->program_name));

  current_task->data_segment_end = align_page((void *)end_of_code);

  u32 ptr = setup_stack(0x90000000, argc, argv);

  jump_usermode((void (*)())(entry), ptr);
  ASSERT_NOT_REACHED;
  return 0;
}

int fork(void) {
  process_t *parent_task = (process_t *)current_task;

  process_t *new_task = create_process(parent_task);

  process_t *tmp_task = (process_t *)ready_queue;
  for (; tmp_task->next;)
    tmp_task = tmp_task->next;

  tmp_task->next = new_task;

  u32 eip = read_eip();

  if (current_task != parent_task) {
    return 0;
  }

  new_task->child_rc = -1;
  new_task->parent = current_task;
  current_task->child = new_task;

  new_task->eip = eip;
  asm("\
	mov %%esp, %0;\
	mov %%ebp, %1;"
      : "=r"(new_task->esp), "=r"(new_task->ebp));
  asm("sti");
  return new_task->pid;
}

int is_halted(process_t *process) {
  for (int i = 0; i < 2; i++)
    if (process->halts[i])
      return 1;

  if (isset_fdhalt(process->read_halt_inode, process->write_halt_inode,
                   process->disconnect_halt_inode)) {
    return 1;
  }
  return 0;
}

extern PageDirectory *active_directory;

process_t *next_task(process_t *c) {
  for (;;) {
    c = c->next;
    if (!c)
      c = ready_queue;
    if (c->incoming_signal)
      break;
    if (c->sleep_until > pit_num_ms())
      continue;
    if (is_halted(c) || c->dead)
      continue;
    break;
  }
  return c;
}

int task_save_state(void) {
  asm("mov %%esp, %0" : "=r"(current_task->esp));
  asm("mov %%ebp, %0" : "=r"(current_task->ebp));

  u32 eip = read_eip();

  if (0x1 == eip) {
    // Should the returned value from read_eip be equal to one it
    // means that we have just switched over to this task after we
    // saved the state(since the switch_task() function changes the
    // eax register to 1).
    return 0;
  }

  current_task->eip = eip;
  return 1;
}

int kill(pid_t pid, int sig) {
  process_t *p = current_task;
  p = p->next;
  if (!p)
    p = ready_queue;
  for (; p->pid != pid;) {
    if (p == current_task)
      break;
    p = p->next;
    if (!p)
      p = ready_queue;
  }
  if (p->pid != pid)
    return -ESRCH;
  p->incoming_signal = sig;
  return 0;
}

void jump_signal_handler(void *func, u32 esp);
void switch_task() {
  if (!current_task)
    return;

  if (0 == task_save_state()) {
    return;
  }

  current_task = next_task((process_t *)current_task);

  active_directory = current_task->cr3;

  if (current_task->incoming_signal) {
    u8 sig = current_task->incoming_signal;
    current_task->incoming_signal = 0;
    asm("mov %0, %%cr3" ::"r"(current_task->cr3->physical_address));

    void *handler = current_task->signal_handlers[sig];
    if (9 == sig) {
      klog("Task recieved SIGKILL", LOG_NOTE);
      exit(0);
    }
    if (!handler) {
      klog("Task recieved unhandeled signal. Killing process.", LOG_WARN);
      exit(1);
    }
    jump_signal_handler(handler, current_task->signal_handler_stack);
  } else {
    asm("			\
        mov %0, %%esp;		\
        mov %1, %%ebp;		\
        mov %2, %%ecx;		\
        mov %3, %%cr3;		\
        mov $0x1, %%eax;	\
        jmp *%%ecx" ::"r"(current_task->esp),
        "r"(current_task->ebp), "r"(current_task->eip),
        "r"(current_task->cr3->physical_address));
  }
}

MemoryMap **get_free_map(void) {
  for (int i = 0; i < 100; i++)
    if (!(current_task->maps[i]))
      return &(current_task->maps[i]);
  assert(0);
  return NULL;
}

void *get_free_virtual_memory(size_t length) {
  void *n =
      (void *)((uintptr_t)(get_current_task()->data_segment_end) + length);

  void *rc = get_current_task()->data_segment_end;
  get_current_task()->data_segment_end = align_page(n);
  return rc;
}

void *allocate_virtual_user_memory(size_t length, int prot, int flags) {
  (void)prot;
  (void)flags;
  void *rc = get_free_virtual_memory(length);
  if ((void *)-1 == rc)
    return (void *)-1;

  mmu_allocate_region(rc, length, MMU_FLAG_RW, NULL);
  return rc;
}

void *user_kernel_mapping(void *kernel_addr, size_t length) {
  void *rc = get_free_virtual_memory(length);
  if ((void *)-1 == rc)
    return (void *)-1;

  mmu_map_directories(rc, NULL, kernel_addr, NULL, length);
  return rc;
}

void *create_physical_mapping(void **physical_addresses, size_t length) {
  void *rc = get_free_virtual_memory(length);
  if ((void *)-1 == rc)
    return (void *)-1;
  int n = (uintptr_t)align_page((void *)length) / 0x1000;
  for (int i = 0; i < n; i++) {
    mmu_map_physical(rc + (i * 0x1000), NULL, physical_addresses[i], 0x1000);
  }
  return rc;
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd,
           size_t offset) {
  (void)addr;
  if (0 == length) {
    kprintf("EINVAL\n");
    return (void *)-EINVAL;
  }

  MemoryMap **ptr = get_free_map();
  if (!ptr) {
    klog("mmap(): No free memory map.", LOG_WARN);
    return (void *)-1;
  }
  *ptr = kmalloc(sizeof(MemoryMap));
  MemoryMap *free_map = *ptr;

  if (fd == -1) {
    void *rc = allocate_virtual_user_memory(length, prot, flags);
    if ((void *)-1 == rc) {
      kprintf("ENOMEM\n");
      return (void *)-ENOMEM;
    }
    free_map->u_address = rc;
    free_map->k_address = NULL;
    free_map->length = length;
    free_map->fd = -1;
    return rc;
  }

  vfs_vm_object_t *vmobject = vfs_get_vm_object(fd, length, offset);
  if (!vmobject) {
    kprintf("ENODEV\n");
    return (void *)-ENODEV;
  }

  if (vmobject->size < length) {
    kprintf("EOVERFLOW\n");
    return (void *)-EOVERFLOW; // TODO: Check if this is the correct
                               // code.
  }

  if (length > vmobject->size)
    length = vmobject->size;
  void *rc = create_physical_mapping(vmobject->object, length);
  free_map->u_address = rc;
  free_map->k_address = NULL;
  free_map->length = length;
  free_map->fd = fd;
  return rc;
}
