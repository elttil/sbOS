#ifndef HALTS_H
#define HALTS_H
#include <fs/vfs.h>
#include <stdint.h>

typedef struct {
  uint8_t *ptr;
  uint8_t active;
} halt_t;

int create_read_fdhalt(vfs_fd_t *fd);
int create_read_inode_halt(vfs_inode_t *inode);
void unset_read_fdhalt(int i);
int create_write_fdhalt(vfs_fd_t *fd);
int create_write_inode_halt(vfs_inode_t *inode);
void unset_write_fdhalt(int i);
int create_disconnect_fdhalt(vfs_fd_t *fd);
void unset_disconnect_fdhalt(int i);
int isset_fdhalt(vfs_inode_t *read_halts[], vfs_inode_t *write_halts[],
                 vfs_inode_t *disconnect_halts[]);
#endif
