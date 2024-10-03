// FIXME: Make sure that the args variabel actually points to something
// valid.
#include <assert.h>
#include <cpu/syscall.h>
#include <drivers/pit.h>
#include <drivers/pst.h>
#include <errno.h>
#include <fcntl.h>
#include <fs/shm.h>
#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <interrupts.h>
#include <kmalloc.h>
#include <math.h>
#include <network/ethernet.h>
#include <network/tcp.h>
#include <network/udp.h>
#include <poll.h>
#include <queue.h>
#include <random.h>
#include <socket.h>
#include <string.h>
#include <timer.h>
#include <typedefs.h>

struct two_args {
  u32 a;
  u32 b;
};

typedef struct SYS_OPEN_PARAMS {
  const char *file;
  int flags;
  int mode;
} __attribute__((packed)) SYS_OPEN_PARAMS;

typedef struct SYS_PREAD_PARAMS {
  int fd;
  void *buf;
  size_t count;
  size_t offset;
} __attribute__((packed)) SYS_PREAD_PARAMS;

typedef struct SYS_READ_PARAMS {
  int fd;
  void *buf;
  size_t count;
} __attribute__((packed)) SYS_READ_PARAMS;

typedef struct SYS_PWRITE_PARAMS {
  int fd;
  const void *buf;
  size_t count;
  size_t offset;
} __attribute__((packed)) SYS_PWRITE_PARAMS;

typedef struct SYS_WRITE_PARAMS {
  int fd;
  const void *buf;
  size_t count;
} __attribute__((packed)) SYS_WRITE_PARAMS;

typedef struct SYS_EXEC_PARAMS {
  const char *path;
  char **argv;
} __attribute__((packed)) SYS_EXEC_PARAMS;

typedef struct SYS_DUP2_PARAMS {
  int org_fd;
  int new_fd;
} __attribute__((packed)) SYS_DUP2_PARAMS;

typedef struct SYS_OPENPTY_PARAMS {
  int *amaster;
  int *aslave;
  char *name;
  /*const struct termios*/ void *termp;
  /*const struct winsize*/ void *winp;
} __attribute__((packed)) SYS_OPENPTY_PARAMS;

typedef struct SYS_POLL_PARAMS {
  struct pollfd *fds;
  size_t nfds;
  int timeout;
} __attribute__((packed)) SYS_POLL_PARAMS;

typedef struct SYS_MMAP_PARAMS {
  void *addr;
  size_t length;
  int prot;
  int flags;
  int fd;
  size_t offset;
} __attribute__((packed)) SYS_MMAP_PARAMS;

typedef struct SYS_SOCKET_PARAMS {
  int domain;
  int type;
  int protocol;
} __attribute__((packed)) SYS_SOCKET_PARAMS;

typedef struct SYS_BIND_PARAMS {
  int sockfd;
  const struct sockaddr *addr;
  socklen_t addrlen;
} __attribute__((packed)) SYS_BIND_PARAMS;

typedef struct SYS_ACCEPT_PARAMS {
  int socket;
  struct sockaddr *address;
  socklen_t *address_len;
} __attribute__((packed)) SYS_ACCEPT_PARAMS;

typedef struct SYS_SHM_OPEN_PARAMS {
  const char *name;
  int oflag;
  mode_t mode;
} __attribute__((packed)) SYS_SHM_OPEN_PARAMS;

typedef struct SYS_FTRUNCATE_PARAMS {
  int fildes;
  size_t length;
} __attribute__((packed)) SYS_FTRUNCATE_PARAMS;

#pragma GCC diagnostic ignored "-Wpedantic"

int syscall_accept(SYS_ACCEPT_PARAMS *args) {
  return accept(args->socket, args->address, args->address_len);
}

int syscall_bind(SYS_BIND_PARAMS *args) {
  return bind(args->sockfd, args->addr, args->addrlen);
}

int syscall_chdir(const char *path) {
  return vfs_chdir(path);
}

int syscall_clock_gettime(clockid_t clock_id, struct timespec *tp) {
  // FIXME: Actually implement this
  if (tp) {
    timer_get(tp);
  }
  return 0;
}

int syscall_fstat(int fd, struct stat *buf) {
  if (!mmu_is_valid_userpointer(buf, sizeof(struct stat))) {
    return -EPERM; // TODO: Is this correct? The spec says nothing about
                   // this case.
  }
  return vfs_fstat(fd, buf);
}

int syscall_ftruncate(int fd, size_t length) {
  return vfs_ftruncate(fd, length);
}

char *syscall_getcwd(char *buf, size_t size) {
  const char *cwd = current_task->current_working_directory;
  size_t len = min(size, strlen(cwd));
  strlcpy(buf, current_task->current_working_directory, len);
  return buf;
}

int syscall_isatty(int fd) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd, NULL);
  if (!fd_ptr) {
    return -EBADF;
  }
  if (!fd_ptr->is_tty) {
    return -ENOTTY;
  }
  return 1;
}

int syscall_kill(int fd, int sig) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd, NULL);
  if (!fd_ptr) {
    return -EBADF;
  }
  if (!fd_ptr->inode->send_signal) {
    return -EBADF;
  }
  return process_signal(fd_ptr, sig);
}

// FIXME: These should be in a shared header file with libc
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int syscall_lseek(int fd, int offset, int whence) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd, NULL);
  if (!fd_ptr) {
    return -EBADF;
  }

  off_t ret_offset = fd_ptr->offset;
  switch (whence) {
  case SEEK_SET:
    ret_offset = offset;
    break;
  case SEEK_CUR:
    ret_offset += offset;
    break;
  case SEEK_END:
    assert(fd_ptr->inode);
    ret_offset = fd_ptr->inode->file_size + offset;
    break;
  default:
    return -EINVAL;
    break;
  }
  fd_ptr->offset = ret_offset;
  return ret_offset;
}

int syscall_mkdir(const char *path, int mode) {
  return vfs_mkdir(path, mode);
}

void *syscall_mmap(SYS_MMAP_PARAMS *args) {
  return mmap(args->addr, args->length, args->prot, args->flags, args->fd,
              args->offset);
}

void syscall_msleep(u32 ms) {
  struct timespec t;
  timer_get(&t);
  current_task->sleep_until = timer_get_uptime() + ms;
  switch_task();
}

int syscall_munmap(void *addr, size_t length) {
  return munmap(addr, length);
}

int syscall_open(const char *file, int flags, mode_t mode) {
  const char *_file = copy_and_allocate_user_string(file);
  if (!_file) {
    return -EFAULT;
  }
  int _flags = flags;
  int _mode = mode;
  int rc = vfs_open(_file, _flags, _mode);
  kfree((void *)_file);
  return rc;
}

int syscall_open_process(int pid) {
  // TODO: Permission check
  process_t *process = (process_t *)ready_queue;
  for (; process; process = process->next) {
    if (pid == process->pid) {
      break;
    }
  }
  if (!process) {
    return -ESRCH;
  }

  vfs_inode_t *inode = vfs_create_inode(
      process->pid, 0 /*type*/, 0 /*has_data*/, 0 /*can_write*/, 1 /*is_open*/,
      0, process /*internal_object*/, 0 /*file_size*/, NULL /*open*/,
      NULL /*create_file*/, NULL /*read*/, NULL /*write*/, NULL /*close*/,
      NULL /*create_directory*/, NULL /*get_vm_object*/, NULL /*truncate*/,
      NULL /*stat*/, process_signal, NULL /*connect*/);
  int rc = vfs_create_fd(0, 0, 0, inode, NULL);
  assert(rc >= 0);
  return rc;
}

int syscall_poll(SYS_POLL_PARAMS *args) {
  struct pollfd *fds = args->fds;
  size_t nfds = args->nfds;
  int timeout = args->timeout;
  return poll(fds, nfds, timeout);
}

int syscall_pwrite(int fd, const char *buf, size_t count, size_t offset) {
  return vfs_pwrite(fd, (char *)buf, count, offset);
}

void syscall_randomfill(void *buffer, u32 size) {
  get_random(buffer, size);
}

size_t syscall_recvfrom(
    int socket, void *buffer, size_t length, int flags,
    struct two_args
        *extra_args /*struct sockaddr *address, socklen_t *address_len*/) {

  struct sockaddr *address = (struct sockaddr *)extra_args->a;
  socklen_t *address_len = (socklen_t *)extra_args->b;

  if (flags & MSG_WAITALL) {
    struct pollfd fds[1];
    fds[0].fd = socket;
    fds[0].events = POLLIN;
    poll(fds, 1, 0);
  }

  u16 data_length;
  socklen_t tmp_socklen;
  vfs_pread(socket, &tmp_socklen, sizeof(socklen_t), 0);
  if (address_len) {
    *address_len = tmp_socklen;
  }
  if (address) {
    vfs_pread(socket, address, tmp_socklen, 0);
  } else {
    // We still have to throwaway the data.
    char devnull[100];
    for (; tmp_socklen;) {
      int rc = vfs_pread(socket, devnull, min(tmp_socklen, 100), 0);
      assert(rc >= 0);
      tmp_socklen -= rc;
    }
  }

  vfs_pread(socket, &data_length, sizeof(data_length), 0);
  // If it is reading less than the packet length that could cause
  // problems as the next read will not be put at a new header. Luckily
  // it seems as if other UNIX systems can discard the rest of the
  // packet if not read.

  // Read in the data requested
  int read_len = min(length, data_length);
  int rc = vfs_pread(socket, buffer, read_len, 0);
  // Discard the rest of the packet
  int rest = data_length - read_len;
  char devnull[100];
  for (; rest;) {
    int rc = vfs_pread(socket, devnull, 100, 0);
    assert(rc >= 0);
    rest -= rc;
  }
  return rc;
}

size_t syscall_sendto(int socket, const void *message, size_t length,
           int flags, struct two_args *extra_args /*
	   const struct sockaddr *dest_addr,
           socklen_t dest_len*/) {
  const struct sockaddr *dest_addr = (const struct sockaddr *)extra_args->a;
  socklen_t dest_len = (socklen_t)extra_args->b;
  (void)dest_len;
  vfs_fd_t *fd = get_vfs_fd(socket, NULL);
  assert(fd);
  SOCKET *s = (SOCKET *)fd->inode->internal_object;
  OPEN_INET_SOCKET *inet = s->child;
  assert(inet);
  struct sockaddr_in in;
  in.sin_addr.s_addr = inet->address;
  in.sin_port = inet->port;
  send_udp_packet(&in, (const struct sockaddr_in *)dest_addr, message, length);
  return length; // FIXME: This is probably not true.
}

int syscall_shm_open(SYS_SHM_OPEN_PARAMS *args) {
  return shm_open(args->name, args->oflag, args->mode);
}

int syscall_sigaction(int sig, const struct sigaction *restrict act,
                      struct sigaction *restrict oact) {
  set_signal_handler(sig, act->sa_handler);
  return 0;
}

int syscall_socket(SYS_SOCKET_PARAMS *args) {
  return socket(args->domain, args->type, args->protocol);
}

u32 syscall_uptime(void) {
  return timer_get_uptime();
}

int syscall_write(int fd, const char *buf, size_t count) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd, NULL);
  if (!fd_ptr) {
    return -EBADF;
  }
  int rc = vfs_pwrite(fd, (char *)buf, count, fd_ptr->offset);
  fd_ptr->offset += rc;
  return rc;
}

int syscall_exec(SYS_EXEC_PARAMS *args) {
  const char *filename = copy_and_allocate_user_string(args->path);

  int argc = 0;
  for (; args->argv[argc];) {
    argc++;
  }

  char *new_argv[argc + 1];
  for (int i = 0; i < argc; i++) {
    new_argv[i] = copy_and_allocate_user_string(args->argv[i]);
  }

  new_argv[argc] = NULL;

  exec(filename, new_argv, 1, 1);
  kfree((void *)filename);
  for (int i = 0; i < argc; i++) {
    kfree(new_argv[i]);
  }
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

int syscall_read(int fd, void *buf, size_t count) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd, NULL);
  if (!fd_ptr) {
    return -EBADF;
  }
  int rc = vfs_pread(fd, buf, count, fd_ptr->offset);
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

int syscall_connect(int sockfd, const struct sockaddr *addr,
                    socklen_t addrlen) {
  return connect(sockfd, addr, addrlen);
}

int syscall_setsockopt(int socket, int level, int option_name,
                       const void *option_value, socklen_t option_len) {
  return setsockopt(socket, level, option_name, option_value, option_len);
}

int tcp_connect(vfs_fd_t *fd, const struct sockaddr *addr, socklen_t addrlen);
int syscall_getpeername(int sockfd, struct sockaddr *restrict addr,
                        socklen_t *restrict addrlen) {
  if (addr) {
    return -EINVAL;
  }

  vfs_fd_t *fd = get_vfs_fd(sockfd, NULL);
  // FIXME: BAD
  assert(fd->inode->connect == tcp_connect);
  struct TcpConnection *con = fd->inode->internal_object;
  if (TCP_STATE_ESTABLISHED != con->state) {
    return -ENOTCONN;
  }

  return 0;
}

int syscall_fcntl(int fd, int cmd, int arg) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd, NULL);
  if (!fd_ptr) {
    return -EBADF;
  }
  if (F_GETFL == cmd) {
    return fd_ptr->flags;
  } else if (F_SETFL == cmd) {
    fd_ptr->flags = arg;
    return 0;
  } else {
    return -EINVAL;
  }
  return 0;
}

int syscall_queue_create(void) {
  return queue_create();
}

int syscall_queue_mod_entries(int fd, int flag, struct queue_entry *entries,
                              int num_entries) {
  return queue_mod_entries(fd, flag, entries, num_entries);
}

int syscall_queue_wait(int fd, struct queue_entry *events, int num_events) {
  return queue_wait(fd, events, num_events);
}

u32 syscall_sendfile(int out_fd, int in_fd, off_t *offset, size_t count,
                     int *error_rc) {
  int is_inf = (0 == count);

  if (offset) {
    if (!mmu_is_valid_userpointer(offset, sizeof(off_t))) {
      return -EFAULT;
    }
  }
  if (error_rc) {
    if (!mmu_is_valid_userpointer(error_rc, sizeof(int))) {
      return -EFAULT;
    }
  }

  vfs_fd_t *in_fd_ptr = get_vfs_fd(in_fd, NULL);
  if (!in_fd_ptr) {
    return -EBADF;
  }

  vfs_fd_t *out_fd_ptr = get_vfs_fd(out_fd, NULL);
  if (!out_fd_ptr) {
    return -EBADF;
  }

  int real_rc = 0;

  off_t read_offset = (offset) ? (*offset) : in_fd_ptr->offset;

  for (; count > 0 || is_inf;) {
    u8 buffer[8192];
    int read_amount = min(sizeof(buffer), count);
    if (is_inf) {
      read_amount = sizeof(buffer);
    }
    int rc = vfs_pread(in_fd, buffer, read_amount, read_offset);
    if (0 > rc) {
      if (error_rc) {
        *error_rc = rc;
      }
      return real_rc;
    }

    if (0 == rc) {
      break;
    }

    int rc2 = vfs_pwrite(out_fd, buffer, rc, out_fd_ptr->offset);
    if (0 > rc2) {
      if (error_rc) {
        *error_rc = rc2;
      }
      return real_rc;
    }
    real_rc += rc2;
    if (!is_inf) {
      count -= rc2;
    }
    out_fd_ptr->offset += rc2;

    read_offset += rc2;
    if (offset) {
      *offset = read_offset;
    } else {
      in_fd_ptr->offset = read_offset;
    }

    if (rc < read_amount) {
      break;
    }
  }
  if (error_rc) {
    *error_rc = 0;
  }
  return real_rc;
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
    (void(*))syscall_fstat,
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
    (void(*))syscall_munmap,
    (void(*))syscall_open_process,
    (void(*))syscall_lseek,
    (void(*))syscall_connect,
    (void(*))syscall_setsockopt,
    (void(*))syscall_getpeername,
    (void(*))syscall_fcntl,
    (void(*))syscall_clock_gettime,
    (void(*))syscall_queue_create,
    (void(*))syscall_queue_mod_entries,
    (void(*))syscall_queue_wait,
    (void(*))syscall_sendfile,
};

void int_syscall(reg_t *r);

void syscalls_init(void) {
  install_handler(int_syscall, INT_32_INTERRUPT_GATE(0x3), 0x80);
}
