#include <sched/scheduler.h>
#ifndef IPC_H
#define IPC_H
#include <stdbool.h>
#include <typedefs.h>

#define IPC_BUFFER_SIZE 0x2000
#define IPC_NUM_DATA 400

struct IpcMessage {
  u8 is_used;
  u32 sender_pid;
  u32 size;
  u8 buffer[IPC_BUFFER_SIZE];
};

struct IpcMailbox {
  u32 read_ptr;
  u32 write_ptr;
  struct IpcMessage data[IPC_NUM_DATA];
};

bool ipc_register_endpoint(u32 endpoint);
int ipc_write_to_process(int pid, u8 *buffer, u32 length);
int ipc_write(int ipc_id, u8 *buffer, u32 length);
int ipc_read(u8 *buffer, u32 length, u32 *sender_pid);
int ipc_has_data(process_t *p);
#endif
