#include <assert.h>
#include <interrupts.h>
#include <ipc.h>
#include <math.h>
#include <sched/scheduler.h>
#include <stdbool.h>
#include <string.h>

struct IpcEndpoint {
  u8 in_use;
  u32 pid;
};

struct IpcEndpoint ipc_endpoints[100];

bool ipc_register_endpoint(u32 endpoint) {
  if (endpoint >= 100) {
    return false;
  }
  if (ipc_endpoints[endpoint].in_use) {
    return false;
  }
  ipc_endpoints[endpoint].in_use = 1;
  ipc_endpoints[endpoint].pid = get_current_task()->pid;
  return true;
}

bool ipc_endpoint_to_pid(u32 endpoint, u32 *pid) {
  if (endpoint >= 100) {
    return false;
  }
  if (!ipc_endpoints[endpoint].in_use) {
    return false;
  }
  *pid = ipc_endpoints[endpoint].pid;
  return true;
}

int ipc_get_mailbox(u32 id, struct IpcMailbox **out) {
  process_t *p;
  if (!get_task_from_pid(id, &p)) {
    return 0;
  }
  *out = &p->ipc_mailbox;
  return 1;
}

int ipc_read(u8 *buffer, u32 length, u32 *sender_pid) {
  struct IpcMailbox *handler = &get_current_task()->ipc_mailbox;

  u32 read_ptr = handler->read_ptr;
  struct IpcMessage *ipc_message = &handler->data[read_ptr];
  for (;;) {
    if (!ipc_message->is_used) {
      if (get_current_task()->is_interrupted) {
        get_current_task()->is_interrupted = 0;
        get_current_task()->is_halted = 0;
        return 0;
      }
      get_current_task()->is_halted = 1;
      switch_task();
      continue;
    }
    break;
  }
  get_current_task()->is_halted = 0;
  disable_interrupts();
  ipc_message->is_used = 0;
  // TODO: Verify sender_pid is a valid address
  if (sender_pid) {
    *sender_pid = ipc_message->sender_pid;
  }

  u32 len = min(length, ipc_message->size);
  memcpy(buffer, ipc_message->buffer, len);

  // Update read_ptr
  read_ptr++;
  if (read_ptr >= IPC_NUM_DATA) {
    read_ptr = 0;
  }
  handler->read_ptr = read_ptr;
  return len;
}

int ipc_write_to_process(int pid, u8 *buffer, u32 length) {
  struct IpcMailbox *handler;
  assert(ipc_get_mailbox(pid, &handler));

  u32 write_ptr = handler->write_ptr;
  struct IpcMessage *ipc_message = &handler->data[write_ptr];
  ipc_message->is_used = 1;
  u32 len = min(IPC_BUFFER_SIZE, length);
  ipc_message->sender_pid = get_current_task()->pid;
  ipc_message->size = len;
  memcpy(ipc_message->buffer, buffer, len);

  // Update write_ptr
  write_ptr++;
  if (write_ptr >= IPC_NUM_DATA) {
    write_ptr = 0;
  }
  handler->write_ptr = write_ptr;
  return len;
}

int ipc_write(int endpoint, u8 *buffer, u32 length) {
  u32 pid;
  assert(ipc_endpoint_to_pid(endpoint, &pid));
  return ipc_write_to_process(pid, buffer, length);
}
