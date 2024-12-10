// TODO: This should support multiple audio sources.
#include <assert.h>
#include <audio.h>
#include <ctype.h>
#include <drivers/ac97.h>
#include <errno.h>
#include <fs/devfs.h>
#include <lib/sv.h>
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

int volume_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  (void)offset;
  (void)fd;
  struct sv string_view = sv_init(buffer, len);
  struct sv rest;
  u64 volume = sv_parse_unsigned_number(string_view, &rest);
  int i = sv_length(string_view) - sv_length(rest);
  if (0 == i) {
    return 0;
  }

  if (volume > 100) {
    volume = 0;
  }

  ac97_set_volume(volume);
  return i;
}

int volume_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  if (offset > 0) {
    return 0;
  }
  (void)fd;
  int volume = ac97_get_volume();
  return min(len, (u64)kbnprintf(buffer, len, "%d", volume));
}

static int add_files(void) {
  devfs_add_file("/audio", NULL, audio_write, NULL, NULL, audio_can_write,
                 FS_TYPE_CHAR_DEVICE);
  devfs_add_file("/volume", volume_read, volume_write, NULL, always_has_data,
                 always_can_write, FS_TYPE_BLOCK_DEVICE);
  return 1;
}

void audio_init(void) {
  ac97_init();
  add_files();
}
