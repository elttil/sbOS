#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <fs/ext2.h>
#include <fs/vfs.h>
#include <halts.h>
#include <mmu.h>
#include <signal.h>

#define MAX_PATH 256
#define KEYBOARD_HALT 0
#define WAIT_CHILD_HALT 1

int fork(void);
int exec(const char *filename, char **argv);
void switch_task();
void tasking_init(void);
void exit(int status);

void *mmap(void *addr, size_t length, int prot, int flags, int fd,
           size_t offset);
int munmap(void *addr, size_t length);
int msync(void *addr, size_t length, int flags);
int kill(pid_t pid, int sig);
void set_signal_handler(int sig, void (*handler)(int));

typedef struct {
  void *u_address;
  void *k_address;
  uint32_t length;
  int fd;
} MemoryMap;

typedef struct Process process_t;

struct Process {
  uint32_t pid;
  char program_name[100];
  char current_working_directory[MAX_PATH];
  uint32_t eip, esp, ebp;
  uint8_t incoming_signal;
  uint32_t signal_handler_stack;
  void *signal_handlers[20];
  PageDirectory *cr3;
  vfs_fd_t *file_descriptors[100];
  vfs_inode_t *read_halt_inode[100];
  vfs_inode_t *write_halt_inode[100];
  vfs_inode_t *disconnect_halt_inode[100];
  uint32_t halts[2];
  struct Halt *halt_list;
  void *data_segment_end;
  process_t *next;
  process_t *parent;
  // TODO: Create a linkedlist of childs so that the parent process
  // can do stuff such as reap zombies and get status.
  process_t *child;
  MemoryMap *maps[100];
  uint32_t sleep_until;
  int child_rc;
  int dead;
};

process_t *get_current_task(void);
int get_free_fd(process_t *p, int allocate);
void free_process(void);
#endif
