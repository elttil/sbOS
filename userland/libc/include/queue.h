#include <stdint.h>

#define QUEUE_MOD_ADD 0
#define QUEUE_MOD_CHANGE 1
#define QUEUE_MOD_DELETE 2

#define QUEUE_WAIT_READ (1 << 0)
#define QUEUE_WAIT_WRITE (1 << 1)
#define QUEUE_WAIT_CLOSE (1 << 2)

struct queue_entry {
  int fd;
  uint8_t listen;
  int data_type;
  void *data;
};

int queue_create(void);
int queue_mod_entries(int fd, int flag, struct queue_entry *entries,
                      int num_entries);
int queue_wait(int fd, struct queue_entry *events, int num_events);
