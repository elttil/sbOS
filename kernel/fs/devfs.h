#ifndef DEVFS_H
#define DEVFS_H
#include <defs.h>
#include <fs/vfs.h>
#include <typedefs.h>

typedef struct devfs_file {
  char *name;
  vfs_inode_t *inode;
} devfs_file;

vfs_inode_t *devfs_mount(void);
vfs_inode_t *devfs_open(const char *file);
int devfs_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd);
int devfs_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd);
void add_stdout(void);
void add_serial(void);
vfs_inode_t *devfs_add_file(
    char *path, int (*read)(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd),
    int (*write)(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd),
    vfs_vm_object_t *(*get_vm_object)(u64 length, u64 offset, vfs_fd_t *fd),
    int (*has_data)(vfs_inode_t *inode), int (*can_write)(vfs_inode_t *inode),
    int type);


int always_has_data(vfs_inode_t *inode);
int always_can_write(vfs_inode_t *inode);
#endif
