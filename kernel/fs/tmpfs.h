#ifndef TMP_H
#define TMP_H
#include <fs/fifo.h>
#include <fs/vfs.h>

#define TMP_BUFFER_SIZE (1024 * 10)

typedef struct {
  FIFO_FILE *fifo;
  uint8_t is_closed;
  vfs_inode_t *read_inode;
} tmp_inode;

void pipe(int fd[2]);
void dual_pipe(int fd[2]);
#endif
