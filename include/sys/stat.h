#ifndef SYS_STAT_H
#define SYS_STAT_H
#include <sys/types.h>
#include <time.h>

#define STAT_REG (1 << 0)
#define STAT_DIR (1 << 1)
#define STAT_FIFO (1 << 2)

#define S_ISREG(_v) (((_v) >> 0) & 1)
#define S_ISDIR(_v) (((_v) >> 1) & 1)
#define S_ISFIFO(_v) (((_v) >> 2) & 1)

struct stat {
  dev_t st_dev;     // Device ID of device containing file.
  ino_t st_ino;     // File serial number.
  mode_t st_mode;   // Mode of file (see below).
  nlink_t st_nlink; // Number of hard links to the file.
  uid_t st_uid;     // User ID of file.
  gid_t st_gid;     // Group ID of file.
  dev_t st_rdev;    // Device ID (if file is character or block special).

  off_t st_size; // For regular files, the file size in bytes.
                 // For symbolic links, the length in bytes of the
                 // pathname contained in the symbolic link.
                 // For a shared memory object, the length in bytes.
                 // For a typed memory object, the length in bytes.
                 // For other file types, the use of this field is
                 // unspecified.

  time_t st_atime; // Last data access timestamp.
  time_t st_mtime; // Last data modification timestamp.
  time_t st_ctime; // Last file status change timestamp.

  blksize_t st_blksize; //  A file system-specific preferred I/O block size
                        // for this object. In some file system types, this
                        // may vary from file to file.

  blkcnt_t st_blocks; // Number of blocks allocated for this object.
};
#ifndef KERNEL
int stat(const char *path, struct stat *buf);
int fstat(int fd, struct stat *buf);
#endif
#endif
