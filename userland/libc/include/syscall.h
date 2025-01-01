#ifndef SYSCALL_H
#define SYSCALL_H
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
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
#define SYS_FSTAT 23
#define SYS_MSLEEP 24
#define SYS_UPTIME 25
#define SYS_MKDIR 26
#define SYS_RECVFROM 27
#define SYS_SENDTO 28
#define SYS_SIGACTION 29
#define SYS_CHDIR 30
#define SYS_GETCWD 31
#define SYS_ISATTY 32
#define SYS_RANDOMFILL 33
#define SYS_MUNMAP 34
#define SYS_LSEEK 35
#define SYS_CONNECT 36
#define SYS_SETSOCKOPT 37
#define SYS_GETPEERNAME 38
#define SYS_FCNTL 39
#define SYS_CLOCK_GETTIME 40
#define SYS_QUEUE_CREATE 41
#define SYS_QUEUE_MOD_ENTRIES 42
#define SYS_QUEUE_WAIT 43
#define SYS_SENDFILE 44
#define SYS_SHM_UNLINK 45
#define SYS_DUP 46
#define SYS_UNLINK 47

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
#endif
