#ifndef DEVFS_H
#define DEVFS_H
#include <defs.h>
#include <fs/vfs.h>
#include <stdint.h>

typedef struct devfs_file {
  char *name;
  vfs_inode_t *inode;
} devfs_file;

vfs_inode_t *devfs_mount(void);
vfs_inode_t *devfs_open(const char *file);
int devfs_read(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd);
int devfs_write(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd);
void add_stdout(void);
void add_serial(void);
vfs_inode_t *devfs_add_file(
    char *path,
    int (*read)(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd),
    int (*write)(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd),
    vfs_vm_object_t *(*get_vm_object)(uint64_t length, uint64_t offset,
                                      vfs_fd_t *fd),
    uint8_t has_data, uint8_t can_write, int type);

#endif
