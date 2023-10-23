#ifndef SYSCALL_H
#define SYSCALL_H
#include "socket.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define SYS_OPEN 0
#define SYS_READ 1
#define SYS_WRITE 2
#define SYS_PREAD 3
#define SYS_PWRITE 4
#define SYS_FORK 5
#define SYS_EXEC 6
#define SYS_GETPID 7
#define SYS_EXIT 8
#define SYS_WAIT 9
#define SYS_BRK 10
#define SYS_SBRK 11
#define SYS_PIPE 12
#define SYS_DUP2 13
#define SYS_CLOSE 14
#define SYS_OPENPTY 15
#define SYS_POLL 16
#define SYS_MMAP 17
#define SYS_ACCEPT 18
#define SYS_BIND 19
#define SYS_SOCKET 20
#define SYS_SHM_OPEN 21
#define SYS_FTRUNCATE 22
#define SYS_STAT 23
#define SYS_MSLEEP 24
#define SYS_UPTIME 25
#define SYS_MKDIR 26

int syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx,
            uint32_t esi, uint32_t edi);
int s_syscall(int sys);

extern int errno;
#define RC_ERRNO(_rc)                                                          \
  {                                                                            \
    int c = _rc;                                                               \
    if (c < 0) {                                                               \
      errno = -(c);                                                            \
      return -1;                                                               \
    }                                                                          \
    return c;                                                                  \
  }

typedef int mode_t;

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

typedef struct SYS_STAT_PARAMS {
  const char *pathname;
  struct stat *statbuf;
} __attribute__((packed)) SYS_STAT_PARAMS;
#endif
