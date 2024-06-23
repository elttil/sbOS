#include <assert.h>
#include <drivers/vbe.h>
#include <fs/devfs.h>
#include <fs/vfs.h>
#include <mmu.h>
#include <stdio.h>

u8 *framebuffer;
u32 framebuffer_physical;
u32 framebuffer_width;
u32 framebuffer_height;
u64 framebuffer_size;

vfs_vm_object_t vbe_vm_object;

#define CHECK_FLAG(flags, bit) ((flags) & (1 << (bit)))
#define min(_a, _b) (((_a) > (_b)) ? (_b) : (_a))

struct DISPLAY_INFO {
  u32 width;
  u32 height;
  u8 bpp;
};

struct DISPLAY_INFO vbe_info;

int display_driver_init(multiboot_info_t *mbi) {
  assert(CHECK_FLAG(mbi->flags, 12));
  framebuffer_width = mbi->framebuffer_width;
  framebuffer_height = mbi->framebuffer_height;

  u32 bits_pp = mbi->framebuffer_bpp;
  u32 bytes_pp = (bits_pp / 8) + (8 - (bits_pp % 8));

  framebuffer_size = bytes_pp * framebuffer_width * framebuffer_height;

  framebuffer_physical = mbi->framebuffer_addr;
  framebuffer =
      mmu_map_frames((void *)(u32)mbi->framebuffer_addr, framebuffer_size);
  if (!framebuffer) {
    return 0;
  }

  vbe_info.width = framebuffer_width;
  vbe_info.height = framebuffer_height;
  vbe_info.bpp = mbi->framebuffer_bpp;
  return 1;
}

vfs_vm_object_t *vbe_get_vm_object(u64 length, u64 offset, vfs_fd_t *fd) {
  (void)fd;
  (void)length;
  (void)offset;
  int n = (uintptr_t)align_page((void *)(u32)framebuffer_size) / 0x1000;
  vbe_vm_object.object = kmalloc(sizeof(void *) * n);
  if (!vbe_vm_object.object) {
    return NULL;
  }
  vbe_vm_object.size = framebuffer_size;

  for (int i = 0; i < n; i++) {
    vbe_vm_object.object[i] = (void *)framebuffer_physical + (i * 0x1000);
  }
  return &vbe_vm_object;
}

int display_info_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  (void)offset;
  int read_len = min(sizeof(struct DISPLAY_INFO), len);
  memcpy(buffer, &vbe_info, read_len);
  return read_len;
}

void add_vbe_device(void) {
  devfs_add_file("/vbe", NULL, NULL, vbe_get_vm_object, always_has_data,
                 always_can_write, FS_TYPE_BLOCK_DEVICE);
  devfs_add_file("/display_info", display_info_read, NULL, NULL,
                 always_has_data, NULL, FS_TYPE_BLOCK_DEVICE);
}
