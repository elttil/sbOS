#include <ipc.h>
#include <syscalls.h>

int syscall_ipc_register_endpoint(u32 id) {
  return ipc_register_endpoint(id);
}

int syscall_ipc_read(u8 *buffer, u32 length, u32 *sender_pid) {
  return ipc_read(buffer, length, sender_pid);
}

int syscall_ipc_write_to_process(int pid, u8 *buffer, u32 length) {
  return ipc_write_to_process(pid, buffer, length);
}

int syscall_ipc_write(int endpoint, u8 *buffer, u32 length) {
  return ipc_write(endpoint, buffer, length);
}
