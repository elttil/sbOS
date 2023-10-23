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
#include <stdint.h>

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
  uint64_t size;
};

struct vfs_mounts {
  char *path;
  vfs_inode_t *local_root;
};

struct dirent {
  unsigned int d_ino;    // File serial number.
  char d_name[PATH_MAX]; // Filename string of entry.
};

struct vfs_fd {
  size_t offset;
  int flags;
  int mode;
  int reference_count; // Number of usages of this file descriptor,
                       // once it reaches zero then the contents can
                       // be freed.
  vfs_inode_t *inode;
};

struct vfs_inode {
  int inode_num;
  int type;
  uint8_t has_data;
  uint8_t can_write;
  uint8_t is_open;
  void *internal_object;
  uint64_t file_size;
  vfs_inode_t *(*open)(const char *path);
  int (*create_file)(const char *path, int mode);
  int (*read)(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd);
  int (*write)(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd);
  void (*close)(vfs_fd_t *fd);
  int (*create_directory)(const char *path, int mode);
  vfs_vm_object_t *(*get_vm_object)(uint64_t length, uint64_t offset,
                                    vfs_fd_t *fd);
};

int vfs_close(int fd);
vfs_fd_t *get_vfs_fd(int fd);
int vfs_open(const char *file, int flags, int mode);
void vfs_mount(char *path, vfs_inode_t *local_root);
int vfs_pwrite(int fd, void *buf, uint64_t count, uint64_t offset);
int raw_vfs_pwrite(vfs_fd_t *vfs_fd, void *buf, uint64_t count,
                   uint64_t offset);
int vfs_pread(int fd, void *buf, uint64_t count, uint64_t offset);
vfs_vm_object_t *vfs_get_vm_object(int fd, uint64_t length, uint64_t offset);
int vfs_dup2(int org_fd, int new_fd);
vfs_inode_t *vfs_internal_open(const char *file);
int vfs_mkdir(const char *path, int mode);
int vfs_create_fd(int flags, int mode, vfs_inode_t *inode, vfs_fd_t **fd);
vfs_inode_t *vfs_create_inode(
    int inode_num, int type, uint8_t has_data, uint8_t can_write,
    uint8_t is_open, void *internal_object, uint64_t file_size,
    vfs_inode_t *(*open)(const char *path),
    int (*create_file)(const char *path, int mode),
    int (*read)(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd),
    int (*write)(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd),
    void (*close)(vfs_fd_t *fd),
    int (*create_directory)(const char *path, int mode),
    vfs_vm_object_t *(*get_vm_object)(uint64_t length, uint64_t offset,
                                      vfs_fd_t *fd));
#endif
