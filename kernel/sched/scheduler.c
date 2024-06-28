#include <assert.h>
#include <cpu/arch_inst.h>
#include <cpu/io.h>
#include <defs.h>
#include <drivers/pit.h>
#include <elf.h>
#include <errno.h>
#include <fs/vfs.h>
#include <interrupts.h>
#include <signal.h>

// FIXME: Use the process_t struct instead or keep this contained in it.
TCB *current_task_TCB;

void switch_to_task(TCB *next_thread);

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

process_t *get_ready_queue(void) {
  return ready_queue;
}

void process_push_restore_context(process_t *p, reg_t r) {
  if (!p) {
    p = current_task;
  }
  reg_t *new_r = kmalloc(sizeof(reg_t));
  memcpy(new_r, &r, sizeof(reg_t));
  stack_push(&p->restore_context_stack, new_r);
  return;
}

void process_pop_restore_context(process_t *p, reg_t *out_r) {
  if (!p) {
    p = current_task;
  }
  reg_t *r = stack_pop(&p->restore_context_stack);
  if (NULL == r) {
    assert(NULL);
  }
  memcpy(out_r, r, sizeof(reg_t));
  kfree(r);
}

void process_push_signal(process_t *p, signal_t s) {
  if (!p) {
    p = current_task;
  }

  if (p->is_halted) {
    p->is_interrupted = 1;
  }

  signal_t *new_signal_entry = kmalloc(sizeof(signal_t));
  memcpy(new_signal_entry, &s, sizeof(signal_t));
  stack_push(&p->signal_stack, new_signal_entry);
}

const signal_t *process_pop_signal(process_t *p) {
  if (!p) {
    p = current_task;
  }
  return stack_pop(&p->signal_stack);
}

int get_task_from_pid(pid_t pid, process_t **out) {
  for (process_t *tmp = ready_queue; tmp; tmp = tmp->next) {
    if (tmp->pid == pid) {
      *out = tmp;
      return 1;
    }
  }
  return 0;
}

void set_signal_handler(int sig, void (*handler)(int)) {
  if (sig >= 20 || sig < 0) {
    return;
  }
  if (9 == sig) {
    return;
  }
  current_task->signal_handlers[sig] = handler;
}

void insert_eip_on_stack(u32 cr3, u32 address, u32 value);

process_t *create_process(process_t *p, u32 esp, u32 eip) {
  process_t *r = kcalloc(1, sizeof(process_t));
  if (!r) {
    return NULL;
  }
  r->tcb = kcalloc(1, sizeof(struct TCB));
  if (!r->tcb) {
    kfree(r);
    return NULL;
  }
  r->pid = next_pid;
  next_pid++;
  if (!p) {
    assert(0 == r->pid);
    strlcpy(r->program_name, "[kernel]", sizeof(current_task->program_name));
  } else {
    strlcpy(r->program_name, "[child]", sizeof(current_task->program_name));
    strcat(r->program_name, p->program_name);
  }

  if (p) {
    if (!relist_clone(&p->file_descriptors, &r->file_descriptors)) {
      kfree(r->tcb);
      kfree(r);
      return NULL;
    }
    for (int i = 0;; i++) {
      vfs_fd_t *out;
      int empty;
      if (!relist_get(&r->file_descriptors, i, (void **)&out, &empty)) {
        if (empty) {
          break;
        }
        continue;
      }
      if (out) {
        out->inode->ref++;
        out->reference_count++;
      }
    }
  } else {
    relist_init(&r->file_descriptors);
  }

  if (p) {
    r->cr3 = clone_directory(p->cr3);
    if (!r->cr3) {
      kfree(r->tcb);
      kfree(r);
      return NULL;
    }
  } else {
    r->cr3 = get_active_pagedirectory();
  }
  r->parent = p;

  r->tcb->CR3 = r->cr3->physical_address;

  list_init(&r->read_list);
  list_init(&r->write_list);
  list_init(&r->disconnect_list);

  if (esp) {
    esp -= 4;
    insert_eip_on_stack(r->cr3->physical_address, esp, eip);
    esp -= 16;
    insert_eip_on_stack(r->cr3->physical_address, esp, 0xF00DBABE);
    r->tcb->ESP = esp;
  }

  if (p) {
    strcpy(r->current_working_directory, p->current_working_directory);
  } else {
    strcpy(r->current_working_directory, "/");
  }
  r->data_segment_end = (p) ? p->data_segment_end : NULL;
  return r;
}

void tasking_init(void) {
  current_task = ready_queue = create_process(NULL, 0, 0);
  assert(current_task);
  current_task_TCB = current_task->tcb;
  current_task->tcb->CR3 = current_task->cr3->physical_address;
}

int i = 0;
void free_process(process_t *p) {
  disable_interrupts();
  // free_process() will purge all contents such as allocated frames
  // out of the current process. This will be called by exit() and
  // exec*().

  // Do a special free for shared memory which avoids labeling
  // underlying frames as "unused".
  for (int i = 0; i < 100; i++) {
    vfs_close_process(p, i);
    if (!p->maps[i]) {
      continue;
    }
    MemoryMap *m = p->maps[i];
    mmu_remove_virtual_physical_address_mapping(m->u_address, m->length);
  }

  list_free(&p->read_list);
  list_free(&p->write_list);
  list_free(&p->disconnect_list);
  relist_free(&p->file_descriptors);
  kfree(p->tcb);

  mmu_free_pagedirectory(p->cr3);
}

void exit_process(process_t *p, int status) {
  assert(p->pid != 1);
  disable_interrupts();
  p->dead = 1;
  if (p->parent) {
    p->parent->halts[WAIT_CHILD_HALT] = 0;
    p->parent->child_rc = status;
  }
  process_t *new_task = p;
  for (; new_task == p;) {
    if (!new_task->next) {
      new_task = ready_queue;
    }
    new_task = new_task->next;
  }

  // Remove current_task from list
  for (process_t *tmp = ready_queue; tmp;) {
    if (tmp == p) // current_task is ready_queue(TODO:
                  // Figure out whether this could even
                  // happen)
    {
      ready_queue = p->next;
      break;
    }
    if (tmp->next == p) {
      tmp->next = tmp->next->next;
      break;
    }
    tmp = tmp->next;
  }
  free_process(p);
  if (current_task == p) {
    switch_task();
  }
}

void exit(int status) {
  exit_process(current_task, status);
  assert(0);
}

u32 setup_stack(u32 stack_pointer, int argc, char **argv, int *err) {
  *err = 0;
  if (!mmu_allocate_region(STACK_LOCATION - STACK_SIZE, STACK_SIZE, MMU_FLAG_RW,
                           NULL)) {
    *err = 1;
    return 0;
  }
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

int exec(const char *filename, char **argv, int dealloc_argv,
         int dealloc_filename) {
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

  strlcpy(current_task->program_name, filename,
          sizeof(current_task->program_name));

  current_task->data_segment_end = align_page((void *)end_of_code);

  int err;
  u32 ptr = setup_stack(0x90000000, argc, argv, &err);
  if (err) {
    return 0;
  }

  if (dealloc_argv) {
    for (int i = 0; i < argc; i++) {
      kfree(argv[i]);
    }
  }
  if (dealloc_filename) {
    kfree((char *)filename);
  }

  jump_usermode((void (*)())(entry), ptr);
  ASSERT_NOT_REACHED;
  return 0;
}

process_t *internal_fork(process_t *parent);
int fork(void) {
  process_t *new_task;
  new_task = internal_fork(current_task);
  if (0 == new_task) {
    return 0;
  }
  if ((process_t *)1 == new_task) {
    return -ENOMEM; // FIXME: This is probably the reason it failed now but is
                    // not the only reason it could fail(at least in the
                    // future).
  }

  process_t *tmp_task = (process_t *)ready_queue;
  for (; tmp_task->next;) {
    tmp_task = tmp_task->next;
  }

  tmp_task->next = new_task;

  new_task->child_rc = -1;
  new_task->parent = current_task;
  current_task->child = new_task;
  return new_task->pid;
}

int isset_fdhalt(process_t *p, int *empty) {
  *empty = 1;
  if (NULL == p) {
    p = current_task;
  }
  if (p->dead) {
    return 1;
  }
  int blocked = 0;
  struct list *read_list = &p->read_list;
  struct list *write_list = &p->write_list;
  struct list *disconnect_list = &p->disconnect_list;

  for (int i = 0;; i++) {
    vfs_inode_t *inode;
    if (!list_get(read_list, i, (void **)&inode)) {
      break;
    }
    *empty = 0;
    if (inode->_has_data) {
      if (inode->_has_data(inode)) {
        return 0;
      }
    }
    blocked = 1;
  }
  for (int i = 0;; i++) {
    vfs_inode_t *inode;
    if (!list_get(write_list, i, (void **)&inode)) {
      break;
    }
    *empty = 0;
    if (inode->_can_write) {
      if (inode->_can_write(inode)) {
        return 0;
      }
    }
    blocked = 1;
  }
  for (int i = 0;; i++) {
    vfs_inode_t *inode;
    if (!list_get(disconnect_list, i, (void **)&inode)) {
      break;
    }
    *empty = 0;
    if (!inode->is_open) {
      return 0;
    }
    blocked = 1;
  }
  return blocked;
}

int is_halted(process_t *process) {
  for (int i = 0; i < 2; i++) {
    if (process->halts[i]) {
      return 1;
    }
  }

  int fd_empty;
  if (isset_fdhalt(process, &fd_empty) && !fd_empty) {
    return 1;
  }
  return 0;
}

extern PageDirectory *active_directory;

process_t *next_task(process_t *s) {
  process_t *c = s;
  c = c->next;
  for (;; c = c->next) {
    if (!c) {
      c = ready_queue;
    }
    if (c->is_interrupted) {
      break;
    }
    if (s == c) {
      // wait_for_interrupt();
    }
    if (c->sleep_until > pit_num_ms()) {
      continue;
    }
    if (c->dead) {
      kprintf("dead process\n");
      continue;
    }
    if (is_halted(c)) {
      continue;
    }
    break;
  }
  return c;
}

void signal_process(process_t *p, int sig) {
  assert(sig < 32);
  kprintf("sending signal to: %x\n", p);
  if (!p->signal_handlers[sig]) {
    if (SIGTERM == sig) {
      kprintf("HAS NO SIGTERM\n");
      exit_process(p, 1 /*TODO: what should the status be?*/);
      ASSERT_NOT_REACHED;
    } else if (SIGSEGV == sig) {
      kprintf("HAS NO SIGSEGV\n");
      exit_process(p, 1 /*TODO: what should the status be?*/);
      ASSERT_NOT_REACHED;
    } else {
      // TODO: Should also exit proess(I think)
      ASSERT_NOT_REACHED;
    }
  }
  signal_t signal = {.handler_ip = (uintptr_t)p->signal_handlers[sig]};
  process_push_signal(p, signal);
}

int process_signal(vfs_fd_t *fd, int sig) {
  process_t *p = fd->inode->internal_object;
  assert(p);
  signal_process(p, sig);
  return 0;
}

int kill(pid_t pid, int sig) {
  process_t *p = current_task;
  p = p->next;
  if (!p) {
    p = ready_queue;
  }
  for (; p->pid != pid;) {
    if (p == current_task) {
      break;
    }
    p = p->next;
    if (!p) {
      p = ready_queue;
    }
  }
  if (p->pid != pid) {
    return -ESRCH;
  }
  signal_process(p, sig);
  return 0;
}

int is_switching_tasks = 0;
void switch_task() {
  if (!current_task) {
    return;
  }

  if (current_task->dead) {
    current_task = current_task->next;
    if (!current_task) {
      current_task = ready_queue;
    }
  } else {
    is_switching_tasks = 1;
    enable_interrupts();
    current_task = next_task((process_t *)current_task);
    disable_interrupts();
    is_switching_tasks = 0;
  }
  active_directory = current_task->cr3;

  switch_to_task(current_task->tcb);
}

MemoryMap **get_free_map(void) {
  for (int i = 0; i < 100; i++) {
    if (!(current_task->maps[i])) {
      return &(current_task->maps[i]);
    }
  }
  assert(0);
  return NULL;
}

void *get_free_virtual_memory(size_t length) {
  void *n =
      (void *)((uintptr_t)(current_task->data_segment_end) + length + 0x1000);

  void *rc = current_task->data_segment_end;
  current_task->data_segment_end = align_page(n);
  return rc;
}

void *allocate_virtual_user_memory(size_t length, int prot, int flags) {
  (void)prot;
  (void)flags;
  void *rc = get_free_virtual_memory(length);
  if (!rc) {
    return NULL;
  }

  if (!mmu_allocate_region(rc, length, MMU_FLAG_RW, NULL)) {
    return NULL;
  }
  return rc;
}

void *create_physical_mapping(void **physical_addresses, size_t length) {
  void *rc = get_free_virtual_memory(length);
  if (!rc) {
    return NULL;
  }
  int n = (uintptr_t)align_page((void *)length) / 0x1000;
  for (int i = 0; i < n; i++) {
    if (!mmu_map_physical(rc + (i * 0x1000), NULL, physical_addresses[i],
                          0x1000)) {
      return NULL;
    }
  }
  return rc;
}

int munmap(void *addr, size_t length) {
  for (int i = 0; i < 100; i++) {
    MemoryMap *m = current_task->maps[i];
    if (!m) {
      continue;
    }
    if (addr == m->u_address) {
      current_task->maps[i] = NULL;
      return 0;
    }
  }
  return 0;
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
    klog(LOG_WARN, "mmap(): No free memory map.");
    return (void *)-1;
  }
  *ptr = kmalloc(sizeof(MemoryMap));
  if (!*ptr) {
    return (void *)-ENOMEM;
  }
  MemoryMap *free_map = *ptr;

  if (-1 == fd) {
    void *rc = allocate_virtual_user_memory(length, prot, flags);
    if (!rc) {
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

  if (length > vmobject->size) {
    length = vmobject->size;
  }
  void *rc = create_physical_mapping(vmobject->object, length);
  if (!rc) {
    kprintf("ENOMEM\n");
    return (void *)-ENOMEM;
  }
  free_map->u_address = rc;
  free_map->k_address = NULL;
  free_map->length = length;
  free_map->fd = fd;
  return rc;
}
