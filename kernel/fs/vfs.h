typedef struct vfs_fd vfs_fd_t;
typedef struct vfs_inode vfs_inode_t;
typedef struct vfs_vm_object vfs_vm_object_t;
typedef struct vfs_mounts vfs_mounts_t;
#ifndef VFS_H
#define VFS_H
#include <limits.h>
#include <sched/scheduler.h>
#include <socket.h>
#include <stddef.h>
#include <sys/stat.h>
#include <typedefs.h>
#include <dirent.h>

// FIXME: Is there some standard value for this?
#define O_NONBLOCK (1 << 0)
#define O_READ (1 << 1)
#define O_WRITE (1 << 2)
#define O_CREAT (1 << 3)
#define O_RDONLY O_READ
#define O_WRONLY O_WRITE
#define O_RDWR (O_WRITE | O_READ)

#define FS_TYPE_FILE 0
#define FS_TYPE_UNIX_SOCKET 1
#define FS_TYPE_CHAR_DEVICE 2
#define FS_TYPE_BLOCK_DEVICE 3
#define FS_TYPE_DIRECTORY 4
#define FS_TYPE_LINK 7

struct vfs_vm_object {
  void *virtual_object;
  void **object;
  u64 size;
};

struct vfs_mounts {
  char *path;
  vfs_inode_t *local_root;
};

struct vfs_fd {
  size_t offset;
  int flags;
  int mode;
  int is_tty;
  int reference_count; // Number of usages of this file descriptor,
                       // once it reaches zero then the contents can
                       // be freed.
  vfs_inode_t *inode;
};

struct vfs_inode {
  int inode_num;
  int type;
  u8 has_data;
  u8 can_write;
  u8 is_open;
  void *internal_object;
  u64 file_size;
  vfs_inode_t *(*open)(const char *path);
  int (*create_file)(const char *path, int mode);
  int (*read)(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd);
  int (*write)(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd);
  void (*close)(vfs_fd_t *fd);
  int (*create_directory)(const char *path, int mode);
  vfs_vm_object_t *(*get_vm_object)(u64 length, u64 offset, vfs_fd_t *fd);
  int (*truncate)(vfs_fd_t *fd, size_t length);
  int (*stat)(vfs_fd_t *fd, struct stat *buf);
};

int vfs_close(int fd);
vfs_fd_t *get_vfs_fd(int fd);
int vfs_open(const char *file, int flags, int mode);
void vfs_mount(char *path, vfs_inode_t *local_root);
int vfs_pwrite(int fd, void *buf, u64 count, u64 offset);
int raw_vfs_pwrite(vfs_fd_t *vfs_fd, void *buf, u64 count, u64 offset);
int raw_vfs_pread(vfs_fd_t *vfs_fd, void *buf, u64 count, u64 offset);
int vfs_pread(int fd, void *buf, u64 count, u64 offset);
vfs_vm_object_t *vfs_get_vm_object(int fd, u64 length, u64 offset);
int vfs_dup2(int org_fd, int new_fd);
vfs_inode_t *vfs_internal_open(const char *file);
int vfs_mkdir(const char *path, int mode);
int vfs_create_fd(int flags, int mode, int is_tty, vfs_inode_t *inode,
                  vfs_fd_t **fd);
int vfs_ftruncate(int fd, size_t length);
int vfs_chdir(const char *path);
int vfs_fstat(int fd, struct stat *buf);
vfs_inode_t *vfs_create_inode(
    int inode_num, int type, u8 has_data, u8 can_write, u8 is_open,
    void *internal_object, u64 file_size,
    vfs_inode_t *(*open)(const char *path),
    int (*create_file)(const char *path, int mode),
    int (*read)(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd),
    int (*write)(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd),
    void (*close)(vfs_fd_t *fd),
    int (*create_directory)(const char *path, int mode),
    vfs_vm_object_t *(*get_vm_object)(u64 length, u64 offset, vfs_fd_t *fd),
    int (*truncate)(vfs_fd_t *fd, size_t length),
    int (*stat)(vfs_fd_t *fd, struct stat *buf));
#endif
