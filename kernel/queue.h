#include <lib/relist.h>
#include <sched/scheduler.h>
#include <typedefs.h>

#define QUEUE_WAIT_READ (1 << 0)
#define QUEUE_WAIT_WRITE (1 << 1)

#define QUEUE_MOD_ADD 0
#define QUEUE_MOD_CHANGE 1
#define QUEUE_MOD_DELETE 2

struct queue_entry {
  int fd;
  u8 listen;
  int data_type;
  void *data;
};

struct queue_list {
  struct relist entries; // Store this as a binary tree(red black trees
                         // are used by epoll) as file
                         // descriptors will be used for lookups
  process_t *process;
};

int queue_create(void);
int queue_mod_entries(int fd, int flag, struct queue_entry *entries,
                      int num_entries);
int queue_wait(int fd, struct queue_entry *events, int num_events);
