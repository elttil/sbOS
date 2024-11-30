// TODO: This should support multiple audio sources.
#include <audio.h>
#include <drivers/ac97.h>
#include <errno.h>
#include <fs/devfs.h>
#include <math.h>

int audio_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  (void)offset;
  (void)fd;
  int rc = ac97_add_pcm(buffer, len);
  if (0 == rc) {
    return -EWOULDBLOCK;
  }
  return rc;
}

int audio_can_write(vfs_inode_t *inode) {
  (void)inode;
  return ac97_can_write();
}

static int add_files(void) {
  devfs_add_file("/audio", NULL, audio_write, NULL, NULL, audio_can_write,
                 FS_TYPE_CHAR_DEVICE);
  return 1;
}

void audio_init(void) {
  ac97_init();
  add_files();
}
