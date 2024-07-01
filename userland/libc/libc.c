#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>

int errno;

char *errno_strings[] = {
    "",
    "Argument list too long.",
    "Permission denied.",
    "Address in use.",
    "Address not available.",
    "Address family not supported.",
    "Resource unavailable, try again (may be the same value as [EWOULDBLOCK]).",
    "Connection already in progress.",
    "Bad file descriptor.",
    "Bad message.",
    "Device or resource busy.",
    "Operation canceled.",
    "No child processes.",
    "Connection aborted.",
    "Connection refused.",
    "Connection reset.",
    "Resource deadlock would occur.",
    "Destination address required.",
    "Mathematics argument out of domain of function.",
    "Reserved.",
    "File exists.",
    "Bad address.",
    "File too large.",
    "Host is unreachable.",
    "Identifier removed.",
    "Illegal byte sequence.",
    "Operation in progress.",
    "Interrupted function.",
    "Invalid argument.",
    "I/O error.",
    "Socket is connected.",
    "Is a directory.",
    "Too many levels of symbolic links.",
    "File descriptor value too large.",
    "Too many links.",
    "Message too large.",
    "Reserved.",
    "Filename too long.",
    "Network is down.",
    "Connection aborted by network.",
    "Network unreachable.",
    "Too many files open in system.",
    "No buffer space available.",
    "No message is available on the STREAM head read "
    "queue. [Option End]",
    "No such device.",
    "No such file or directory.",
    "Executable file format error.",
    "No locks available.",
    "Reserved.",
    "Not enough space.",
    "No message of the desired type.",
    "Protocol not available.",
    "No space left on device.",
    "No STREAM resources.",
    "Not a STREAM.",
    "Functionality not supported.",
    "The socket is not connected.",
    "Not a directory or a symbolic link to a directory.",
    "Directory not empty.",
    "State not recoverable.",
    "Not a socket.",
    "Not supported (may be the same value as [EOPNOTSUPP]).",
    "Inappropriate I/O control operation.",
    "No such device or address.",
    "Operation not supported on socket (may be the same value as [ENOTSUP]).",
    "Value too large to be stored in data type.",
    "Previous owner died.",
    "Operation not permitted.",
    "Broken pipe.",
    "Protocol error.",
    "Protocol not supported.",
    "Protocol wrong type for socket.",
    "Result too large.",
    "Read-only file system.",
    "Invalid seek.",
    "No such process.",
    "Reserved.",
    "Stream ioctl() timeout. [Option End]",
    "Connection timed out.",
    "Text file busy.",
    "Operation would block (may be the same value as [EAGAIN]).",
    "Cross-device link. ",
};

#define ASSERT_NOT_REACHED assert(0)

#define TAB_SIZE 8

FILE *__stdin_FILE;
FILE *__stdout_FILE;
FILE *__stderr_FILE;
#define stdin __stdin_FILE
#define stdout __stdout_FILE
#define stderr __stderr_FILE

void _libc_setup(void) {
  __stdin_FILE = calloc(1, sizeof(FILE));
  __stdin_FILE->write = write_fd;
  __stdin_FILE->read = read_fd;
  __stdin_FILE->seek = NULL;
  __stdin_FILE->is_eof = 0;
  __stdin_FILE->has_error = 0;
  __stdin_FILE->cookie = NULL;
  __stdin_FILE->fd = 0;
  __stdin_FILE->fflush = fflush_fd;

  __stdout_FILE = calloc(1, sizeof(FILE));
  __stdout_FILE->write = write_fd;
  __stdout_FILE->read = read_fd;
  __stdout_FILE->is_eof = 0;
  __stdout_FILE->has_error = 0;
  __stdout_FILE->seek = NULL;
  __stdout_FILE->cookie = NULL;
  __stdout_FILE->fd = 1;
  __stdout_FILE->fflush = fflush_fd;

  __stderr_FILE = calloc(1, sizeof(FILE));
  __stderr_FILE->write = write_fd;
  __stderr_FILE->read = read_fd;
  __stderr_FILE->is_eof = 0;
  __stderr_FILE->has_error = 0;
  __stdout_FILE->seek = NULL;
  __stderr_FILE->cookie = NULL;
  __stderr_FILE->fd = 2;
  __stderr_FILE->fflush = fflush_fd;
}

// Functions preserve the registers ebx, esi, edi, ebp, and esp; while
// eax, ecx, edx are scratch registers.

// Syscall: eax ebx ecx edx esi edi
int syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx,
            uint32_t esi, uint32_t edi);

int pipe(int fd[2]) {
  return syscall(SYS_PIPE, (u32)fd, 0, 0, 0, 0);
}

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/strerror.html
char *strerror(int errnum) {
  // The strerror() function shall map the error number in errnum to a
  // locale-dependent error message string and shall return a pointer to it.
  return errno_strings[errnum];
}

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/perror.html
void perror(const char *s) {
  // The perror() function shall map the error number accessed through the
  // symbol errno to a language-dependent error message, which shall be written
  // to the standard error stream as follows:

  // (First (if s is not a null pointer and the character pointed to
  // by s is not the null byte),
  if (s && *s != '\0') {
    // the string pointed to by s
    // followed by a <colon> and a <space>.
    printf("%s: ", s);
  }

  // Then an error message string followed by a <newline>.
  // The contents of the error message strings shall be the same as those
  // returned by strerror() with argument errno.
  printf("%s\n", strerror(errno));
}

int close(int fd) {
  return syscall(SYS_CLOSE, (u32)fd, 0, 0, 0, 0);
}

int execv(char *path, char **argv) {
  struct SYS_EXEC_PARAMS args = {.path = path, .argv = argv};
  return syscall(SYS_EXEC, (u32)&args, 0, 0, 0, 0);
}

int s_syscall(int sys);

int wait(int *stat_loc) {
  return syscall(SYS_WAIT, (u32)stat_loc, 0, 0, 0, 0);
}

void exit(int status) {
  syscall(SYS_EXIT, (u32)status, 0, 0, 0, 0);
}

int pread(int fd, void *buf, size_t count, size_t offset) {
  struct SYS_PREAD_PARAMS args = {
      .fd = fd,
      .buf = buf,
      .count = count,
      .offset = offset,
  };
  RC_ERRNO(syscall(SYS_PREAD, (u32)&args, 0, 0, 0, 0));
}

int read(int fd, void *buf, size_t count) {
  RC_ERRNO(syscall(SYS_READ, fd, buf, count, 0, 0));
}

int dup2(int org_fd, int new_fd) {
  struct SYS_DUP2_PARAMS args = {
      .org_fd = org_fd,
      .new_fd = new_fd,
  };
  RC_ERRNO(syscall(SYS_DUP2, (u32)&args, 0, 0, 0, 0));
}

int fork(void) {
  return s_syscall(SYS_FORK);
}

int brk(void *addr) {
  return syscall(SYS_BRK, addr, 0, 0, 0, 0);
}

void *sbrk(intptr_t increment) {
  return (void *)syscall(SYS_SBRK, (void *)increment, 0, 0, 0, 0);
}

int poll(struct pollfd *fds, size_t nfds, int timeout) {
  SYS_POLL_PARAMS args = {
      .fds = fds,
      .nfds = nfds,
      .timeout = timeout,
  };
  RC_ERRNO(syscall(SYS_POLL, (u32)&args, 0, 0, 0, 0));
}

int socket(int domain, int type, int protocol) {
  SYS_SOCKET_PARAMS args = {
      .domain = domain,
      .type = type,
      .protocol = protocol,
  };
  RC_ERRNO(syscall(SYS_SOCKET, (u32)&args, 0, 0, 0, 0));
}

int accept(int socket, struct sockaddr *address, socklen_t *address_len) {
  SYS_ACCEPT_PARAMS args = {
      .socket = socket,
      .address = address,
      .address_len = address_len,
  };
  RC_ERRNO(syscall(SYS_ACCEPT, (u32)&args, 0, 0, 0, 0));
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  SYS_BIND_PARAMS args = {
      .sockfd = sockfd,
      .addr = addr,
      .addrlen = addrlen,
  };
  RC_ERRNO(syscall(SYS_BIND, (u32)&args, 0, 0, 0, 0));
}

int shm_open(const char *name, int oflag, mode_t mode) {
  SYS_SHM_OPEN_PARAMS args = {
      .name = name,
      .oflag = oflag,
      .mode = mode,
  };
  RC_ERRNO(syscall(SYS_SHM_OPEN, (u32)&args, 0, 0, 0, 0));
}
