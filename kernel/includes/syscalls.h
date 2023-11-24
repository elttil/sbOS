#include <socket.h>
#include <fs/vfs.h>
#include <signal.h>
#include <socket.h>
#include <stddef.h>
#include <sys/types.h>
#include <time.h>
#include <typedefs.h>
#include <types.h>

typedef struct SYS_ACCEPT_PARAMS {
  int socket;
  struct sockaddr *address;
  socklen_t *address_len;
} __attribute__((packed)) SYS_ACCEPT_PARAMS;

int syscall_accept(SYS_ACCEPT_PARAMS *args);
int syscall_open(const char *file, int flags, mode_t mode);

void syscall_randomfill(void *buffer, u32 size);

typedef struct SYS_BIND_PARAMS {
  int sockfd;
  const struct sockaddr *addr;
  socklen_t addrlen;
} __attribute__((packed)) SYS_BIND_PARAMS;

int syscall_bind(SYS_BIND_PARAMS *args);
int syscall_chdir(const char *path);

typedef struct SYS_CLOCK_GETTIME_PARAMS {
  clockid_t clk;
  struct timespec *ts;
} __attribute__((packed)) SYS_CLOCK_GETTIME_PARAMS;

int syscall_clock_gettime(SYS_CLOCK_GETTIME_PARAMS *args);
int syscall_ftruncate(int fd, size_t length);

char *syscall_getcwd(char *buf, size_t size);
int syscall_kill(pid_t pid, int sig);
int syscall_mkdir(const char *path, int mode);

typedef struct SYS_MMAP_PARAMS {
  void *addr;
  size_t length;
  int prot;
  int flags;
  int fd;
  size_t offset;
} __attribute__((packed)) SYS_MMAP_PARAMS;

void *syscall_mmap(SYS_MMAP_PARAMS *args);
#ifndef MSLEEP_H
#define MSLEEP_H
void syscall_msleep(u32 ms);
#endif

typedef struct SYS_POLL_PARAMS {
  struct pollfd *fds;
  size_t nfds;
  int timeout;
} __attribute__((packed)) SYS_POLL_PARAMS;

int syscall_poll(SYS_POLL_PARAMS *args);

struct two_args {
  u32 a;
  u32 b;
};

size_t syscall_recvfrom(
    int socket, void *buffer, size_t length, int flags,
    struct two_args
        *extra_args /*struct sockaddr *address, socklen_t *address_len*/);

struct t_two_args {
  u32 a;
  u32 b;
};
size_t syscall_sendto(int socket, const void *message, size_t length,
           int flags, struct t_two_args *extra_args /*
	   const struct sockaddr *dest_addr,
           socklen_t dest_len*/);
#ifndef SYS_SHM_H
#define SYS_SHM_H

typedef struct SYS_SHM_OPEN_PARAMS {
  const char *name;
  int oflag;
  mode_t mode;
} __attribute__((packed)) SYS_SHM_OPEN_PARAMS;

int syscall_shm_open(SYS_SHM_OPEN_PARAMS *args);
#endif
int syscall_sigaction(int sig, const struct sigaction *restrict act,
                      struct sigaction *restrict oact);

typedef struct SYS_SOCKET_PARAMS {
  int domain;
  int type;
  int protocol;
} __attribute__((packed)) SYS_SOCKET_PARAMS;

int syscall_socket(SYS_SOCKET_PARAMS *args);

typedef struct SYS_STAT_PARAMS {
  const char *pathname;
  struct stat *statbuf;
} __attribute__((packed)) SYS_STAT_PARAMS;

int syscall_stat(SYS_STAT_PARAMS *args);
u32 syscall_uptime(void);
int syscall_isatty(int fd);
