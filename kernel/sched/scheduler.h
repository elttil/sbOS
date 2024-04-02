typedef struct Process process_t;
#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <fs/ext2.h>
#include <fs/vfs.h>
#include <ipc.h>
#include <lib/list.h>
#include <lib/stack.h>
#include <mmu.h>
#include <signal.h>
#include <stdbool.h>

#define MAX_PATH 256
#define KEYBOARD_HALT 0
#define WAIT_CHILD_HALT 1

extern process_t *current_task;
extern process_t *ready_queue;

int fork(void);
int exec(const char *filename, char **argv, int dealloc_argv,
         int dealloc_filename);
void switch_task(void);
void tasking_init(void);
void exit(int status);
int process_signal(vfs_fd_t *fd, int sig);

void *mmap(void *addr, size_t length, int prot, int flags, int fd,
           size_t offset);
int munmap(void *addr, size_t length);
int msync(void *addr, size_t length, int flags);
int kill(pid_t pid, int sig);
void set_signal_handler(int sig, void (*handler)(int));

typedef struct {
  void *u_address;
  void *k_address;
  u32 length;
  int fd;
} MemoryMap;

typedef struct {
  uintptr_t handler_ip;
} signal_t;

typedef struct TCB {
  uint32_t ESP;
  uint32_t CR3;
  uint32_t ESP0;
} __attribute__((packed)) TCB;

void process_push_restore_context(process_t *p, reg_t r);
void process_pop_restore_context(process_t *p, reg_t *out_r);
void process_push_signal(process_t *p, signal_t s);
const signal_t *process_pop_signal(process_t *p);

struct Process {
  u32 pid;
  char program_name[100];
  char current_working_directory[MAX_PATH];
  void *signal_handlers[32];
  void *interrupt_handler;
  PageDirectory *cr3;
  struct IpcMailbox ipc_mailbox;
  vfs_fd_t *file_descriptors[100];

  struct list read_list;
  struct list write_list;
  struct list disconnect_list;

  struct list tcp_sockets;
  struct list tcp_listen;

  struct list event_queue;

  struct stack restore_context_stack;
  struct stack signal_stack;

  u32 halts[2];
  struct Halt *halt_list;
  void *data_segment_end;
  process_t *next;
  process_t *parent;

  TCB *tcb;

  int is_interrupted;
  int is_halted;

  // TODO: Create a linkedlist of childs so that the parent process
  // can do stuff such as reap zombies and get status.
  process_t *child;
  MemoryMap *maps[100];
  u32 sleep_until;
  int child_rc;
  int dead;
};

bool get_task_from_pid(u32 pid, process_t **out);
int get_free_fd(process_t *p, int allocate);
void free_process(process_t *p);
void *get_free_virtual_memory(size_t length);
#endif
