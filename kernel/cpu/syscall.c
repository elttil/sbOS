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
  for (int i = 0; i < argc; i++)
    new_argv[i] = copy_and_allocate_user_string(args->argv[i]);

  new_argv[argc] = NULL;

  exec(filename, new_argv);
  kfree((void *)filename);
  for (int i = 0; i < argc; i++)
    kfree(new_argv[i]);
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

int syscall_read(SYS_READ_PARAMS *args) {
  vfs_fd_t *fd = get_vfs_fd(args->fd);
  if (!fd)
    return -EBADF;
  int rc = vfs_pread(args->fd, args->buf, args->count, fd->offset);
  fd->offset += rc;
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
  if (!get_current_task()->child) {
    if (status)
      *status = -1;
    return;
  }
  if (get_current_task()->child->dead) {
    if (status)
      *status = get_current_task()->child_rc;
    return;
  }
  get_current_task()->halts[WAIT_CHILD_HALT] = 1;
  switch_task();
  if (status)
    *status = get_current_task()->child_rc;
}

int syscall_fork(void) {
  return fork();
}

int syscall_getpid(void) {
  return get_current_task()->pid;
}

void *align_page(void *a);

int syscall_brk(void *addr) {
  void *end = get_current_task()->data_segment_end;
  if (!mmu_allocate_region(end, addr - end, MMU_FLAG_RW, NULL))
    return -ENOMEM;
  get_current_task()->data_segment_end = align_page(addr);
  return 0;
}

void *syscall_sbrk(uintptr_t increment) {
  disable_interrupts();
  void *rc = get_current_task()->data_segment_end;
  void *n =
      (void *)((uintptr_t)(get_current_task()->data_segment_end) + increment);
  int rc2;
  if (0 > (rc2 = syscall_brk(n)))
    return (void *)rc2;
  return rc;
}

int syscall_close(int fd) {
  return vfs_close(fd);
}

int syscall_openpty(SYS_OPENPTY_PARAMS *args) {
  assert(is_valid_userpointer(args, sizeof(SYS_OPENPTY_PARAMS)));
  return openpty(args->amaster, args->aslave, args->name, args->termp,
                 args->winp);
}

int (*syscall_functions[])() = {
    (void(*))syscall_open,
    (void(*))syscall_read,
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
};

void syscall_function_handler(u32 eax, u32 arg1, u32 arg2, u32 arg3, u32 arg4,
                              u32 arg5, u32 ebp, u32 esp) {
  assert(eax < sizeof(syscall_functions) / sizeof(syscall_functions[0]));
  syscall_functions[eax](arg1, arg2, arg3, arg4, arg5);
}

void int_syscall(reg_t *r) {
  u32 syscall = r->eax;
  assert(syscall < sizeof(syscall_functions) / sizeof(syscall_functions[0]));
  r->eax = syscall_functions[syscall](r->ebx, r->ecx, r->edx, r->esi, r->edi);
}

void syscalls_init(void) {
  install_handler(int_syscall, INT_32_INTERRUPT_GATE(0x3), 0x80);
}
