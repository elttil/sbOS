// FIXME: Make sure that the args variabel actually points to something
// valid.
#include <assert.h>
#include <cpu/syscall.h>
#include <drivers/pst.h>
#include <errno.h>
#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <interrupts.h>
#include <kmalloc.h>
#include <network/ethernet.h>
#include <socket.h>
#include <string.h>
#include <syscalls.h>
#include <typedefs.h>

#pragma GCC diagnostic ignored "-Wpedantic"

int syscall_exec(SYS_EXEC_PARAMS *args) {
  const char *filename = copy_and_allocate_user_string(args->path);

  int argc = 0;
  for (; args->argv[argc];) {
    argc++;
  }

  char **new_argv = kallocarray(argc + 1, sizeof(char *));
  for (int i = 0; i < argc; i++) {
    new_argv[i] = copy_and_allocate_user_string(args->argv[i]);
  }

  new_argv[argc] = NULL;

  exec(filename, new_argv);
  kfree((void *)filename);
  for (int i = 0; i < argc; i++) {
    kfree(new_argv[i]);
  }
  kfree(new_argv);
  return -1;
}

void syscall_tmp_handle_packet(void *packet, u32 len) {
  handle_ethernet((u8 *)packet, len);
}

int syscall_pipe(int fd[2]) {
  pipe(fd); // FIXME: Error checking
  return 0;
}

int syscall_pread(SYS_PREAD_PARAMS *args) {
  return vfs_pread(args->fd, args->buf, args->count, args->offset);
}

int syscall_mread(int fd, void *buf, size_t count, int blocking) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd, NULL);
  if (!fd_ptr) {
    return -EBADF;
  }
  int rc = vfs_pmread(fd, buf, count, blocking, fd_ptr->offset);
  fd_ptr->offset += rc;
  return rc;
}

int syscall_dup2(SYS_DUP2_PARAMS *args) {
  return vfs_dup2(args->org_fd, args->new_fd);
}

void syscall_exit(int status) {
  exit(status);
  assert(0);
}

void syscall_wait(int *status) {
  disable_interrupts();
  if (!current_task->child) {
    if (status) {
      *status = -1;
    }
    return;
  }
  if (current_task->child->dead) {
    if (status) {
      *status = current_task->child_rc;
    }
    return;
  }
  do {
    current_task->is_halted = 1;
    current_task->halts[WAIT_CHILD_HALT] = 1;
    switch_task();
    if (current_task->is_interrupted) {
      break;
    }
  } while (current_task->halts[WAIT_CHILD_HALT]);
  if (current_task->is_interrupted) {
    current_task->is_interrupted = 0;
    return;
  }
  if (status) {
    *status = current_task->child_rc;
  }
}

int syscall_fork(void) {
  return fork();
}

int syscall_getpid(void) {
  return current_task->pid;
}

void *align_page(void *a);

int syscall_brk(void *addr) {
  void *end = current_task->data_segment_end;
  if (!mmu_allocate_region(end, addr - end, MMU_FLAG_RW, NULL)) {
    return -ENOMEM;
  }
  current_task->data_segment_end = align_page(addr);
  return 0;
}

void *syscall_sbrk(uintptr_t increment) {
  disable_interrupts();
  void *rc = current_task->data_segment_end;
  void *n = (void *)((uintptr_t)(current_task->data_segment_end) + increment);
  int rc2;
  if (0 > (rc2 = syscall_brk(n))) {
    return (void *)rc2;
  }
  return rc;
}

int syscall_close(int fd) {
  return vfs_close(fd);
}

int syscall_openpty(SYS_OPENPTY_PARAMS *args) {
  assert(mmu_is_valid_userpointer(args, sizeof(SYS_OPENPTY_PARAMS)));
  return openpty(args->amaster, args->aslave, args->name, args->termp,
                 args->winp);
}

u32 syscall_tcp_connect(u32 ip, u16 port, int *error) {
  // TODO: Make sure error is a user address
  return tcp_connect_ipv4(ip, port, error);
}

int syscall_tcp_write(u32 socket, const u8 *buffer, u32 len, u64 *out) {
  // TODO: Make sure out is a user address
  return tcp_write(socket, buffer, len, out);
}

int syscall_tcp_read(u32 socket, u8 *buffer, u32 buffer_size, u64 *out) {
  // TODO: Make sure out is a user address
  return tcp_read(socket, buffer, buffer_size, out);
}

int (*syscall_functions[])() = {
    (void(*))syscall_open,
    (void(*))syscall_mread,
    (void(*))syscall_write,
    (void(*))syscall_pread,
    (void(*))syscall_pwrite,
    (void(*))syscall_fork,
    (void(*))syscall_exec,
    (void(*))syscall_getpid,
    (void(*))syscall_exit,
    (void(*))syscall_wait,
    (void(*))syscall_brk,
    (void(*))syscall_sbrk,
    (void(*))syscall_pipe,
    (void(*))syscall_dup2,
    (void(*))syscall_close,
    (void(*))syscall_openpty,
    (void(*))syscall_poll,
    (void(*))syscall_mmap,
    (void(*))syscall_accept,
    (void(*))syscall_bind,
    (void(*))syscall_socket,
    (void(*))syscall_shm_open,
    (void(*))syscall_ftruncate,
    (void(*))syscall_stat,
    (void(*))syscall_msleep,
    (void(*))syscall_uptime,
    (void(*))syscall_mkdir,
    (void(*))syscall_recvfrom,
    (void(*))syscall_sendto,
    (void(*))syscall_kill,
    (void(*))syscall_sigaction,
    (void(*))syscall_chdir,
    (void(*))syscall_getcwd,
    (void(*))syscall_isatty,
    (void(*))syscall_randomfill,
    (void(*))syscall_ipc_register_endpoint,
    (void(*))syscall_ipc_read,
    (void(*))syscall_ipc_write,
    (void(*))syscall_ipc_write_to_process,
    (void(*))syscall_outw,
    (void(*))syscall_inl,
    (void(*))syscall_outl,
    (void(*))syscall_map_frames,
    (void(*))syscall_virtual_to_physical,
    (void(*))syscall_install_irq,
    (void(*))syscall_tmp_handle_packet,

    (void(*))syscall_tcp_connect,
    (void(*))syscall_tcp_write,
    (void(*))syscall_tcp_read,

    (void(*))syscall_queue_create,
    (void(*))syscall_queue_add,
    (void(*))syscall_queue_wait,
    (void(*))syscall_munmap,
};

void int_syscall(reg_t *r);

void syscalls_init(void) {
  install_handler(int_syscall, INT_32_INTERRUPT_GATE(0x3), 0x80);
}
