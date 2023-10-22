#include <assert.h>
#include <errno.h>
#include <stdio.h>
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

// Functions preserve the registers ebx, esi, edi, ebp, and esp; while
// eax, ecx, edx are scratch registers.

// Syscall: eax ebx ecx edx esi edi
int syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx,
            uint32_t esi, uint32_t edi) {
  asm volatile("push %edi\n"
               "push %esi\n"
               "push %ebx\n"
               "mov 0x1C(%ebp), %edi\n"
               "mov 0x18(%ebp), %esi\n"
               "mov 0x14(%ebp), %edx\n"
               "mov 0x10(%ebp), %ecx\n"
               "mov 0xc(%ebp), %ebx\n"
               "mov 0x8(%ebp), %eax\n"
               "int $0x80\n"
               "pop %ebx\n"
               "pop %esi\n"
               "pop %edi\n");
}

int pipe(int fd[2]) { return syscall(SYS_PIPE, fd, 0, 0, 0, 0); }

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

int open(const char *file, int flags, int mode) {
  struct SYS_OPEN_PARAMS args = {
      .file = file,
      .flags = flags,
      .mode = mode,
  };
  RC_ERRNO(syscall(SYS_OPEN, &args, 0, 0, 0, 0));
}

int close(int fd) { return syscall(SYS_CLOSE, (void *)fd, 0, 0, 0, 0); }

int execv(char *path, char **argv) {
  struct SYS_EXEC_PARAMS args = {.path = path, .argv = argv};
  return syscall(SYS_EXEC, &args, 0, 0, 0, 0);
}
/*
int syscall(int sys, void *args) {
  asm volatile("push %ebx\n"
               "mov 0xc(%ebp), %ebx\n"
               "mov 0x8(%ebp), %eax\n"
               "int $0x80\n"
               "pop %ebx\n");
}*/

int s_syscall(int sys) {
  asm volatile("movl %0, %%eax\n"
               "int $0x80\n" ::"r"((uint32_t)sys));
}

int write(int fd, const char *buf, size_t count) {
	/*
  struct SYS_WRITE_PARAMS args = {
      .fd = fd,
      .buf = buf,
      .count = count,
  };*/
  return syscall(SYS_WRITE, fd, buf, count, 0, 0);
}

int pwrite(int fd, const char *buf, size_t count, size_t offset) {
  struct SYS_PWRITE_PARAMS args = {
      .fd = fd,
      .buf = buf,
      .count = count,
      .offset = offset,
  };
  return syscall(SYS_PWRITE, &args, 0, 0, 0, 0);
}

int wait(int *stat_loc) { return syscall(SYS_WAIT, stat_loc, 0, 0, 0, 0); }

void exit(int status) { syscall(SYS_EXIT, (void *)status, 0, 0, 0, 0); }

int pread(int fd, void *buf, size_t count, size_t offset) {
  struct SYS_PREAD_PARAMS args = {
      .fd = fd,
      .buf = buf,
      .count = count,
      .offset = offset,
  };
  RC_ERRNO(syscall(SYS_PREAD, &args, 0, 0, 0, 0));
}

int read(int fd, void *buf, size_t count) {
  struct SYS_READ_PARAMS args = {
      .fd = fd,
      .buf = buf,
      .count = count,
  };
  RC_ERRNO(syscall(SYS_READ, &args, 0, 0, 0, 0));
}

int dup2(int org_fd, int new_fd) {
  struct SYS_DUP2_PARAMS args = {
      .org_fd = org_fd,
      .new_fd = new_fd,
  };
  RC_ERRNO(syscall(SYS_DUP2, &args, 0, 0, 0, 0));
}

int fork(void) { return s_syscall(SYS_FORK); }

void dputc(int fd, const char c) { pwrite(fd, &c, 1, 0); }

int brk(void *addr) { return syscall(SYS_BRK, addr, 0, 0, 0, 0); }

void *sbrk(intptr_t increment) {
  return (void *)syscall(SYS_SBRK, (void *)increment, 0, 0, 0, 0);
}

int poll(struct pollfd *fds, size_t nfds, int timeout) {
  SYS_POLL_PARAMS args = {
      .fds = fds,
      .nfds = nfds,
      .timeout = timeout,
  };
  RC_ERRNO(syscall(SYS_POLL, &args, 0, 0, 0, 0));
}

int socket(int domain, int type, int protocol) {
  SYS_SOCKET_PARAMS args = {
      .domain = domain,
      .type = type,
      .protocol = protocol,
  };
  RC_ERRNO(syscall(SYS_SOCKET, &args, 0, 0, 0, 0));
}

int accept(int socket, struct sockaddr *address, socklen_t *address_len) {
  SYS_ACCEPT_PARAMS args = {
      .socket = socket,
      .address = address,
      .address_len = address_len,
  };
  RC_ERRNO(syscall(SYS_ACCEPT, &args, 0, 0, 0, 0));
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  SYS_BIND_PARAMS args = {
      .sockfd = sockfd,
      .addr = addr,
      .addrlen = addrlen,
  };
  RC_ERRNO(syscall(SYS_BIND, &args, 0, 0, 0, 0));
}

int shm_open(const char *name, int oflag, mode_t mode) {
  SYS_SHM_OPEN_PARAMS args = {
      .name = name,
      .oflag = oflag,
      .mode = mode,
  };
  RC_ERRNO(syscall(SYS_SHM_OPEN, &args, 0, 0, 0, 0));
}

int ftruncate(int fildes, uint64_t length) {
  SYS_FTRUNCATE_PARAMS args = {.fildes = fildes, .length = length};
  RC_ERRNO(syscall(SYS_FTRUNCATE, &args, 0, 0, 0, 0));
}
