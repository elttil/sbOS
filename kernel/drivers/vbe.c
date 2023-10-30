#include <assert.h>
#include <drivers/vbe.h>
#include <fs/devfs.h>
#include <fs/vfs.h>
#include <mmu.h>
#include <stdio.h>

uint8_t *framebuffer;
uint32_t framebuffer_physical;
uint32_t framebuffer_width;
uint32_t framebuffer_height;
uint64_t framebuffer_size;

vfs_vm_object_t vbe_vm_object;

#define CHECK_FLAG(flags, bit) ((flags) & (1 << (bit)))
#define min(_a, _b) (((_a) > (_b)) ? (_b) : (_a))

struct DISPLAY_INFO {
  uint32_t width;
  uint32_t height;
  uint8_t bpp;
};

struct DISPLAY_INFO vbe_info;

void display_driver_init(multiboot_info_t *mbi) {
  assert(CHECK_FLAG(mbi->flags, 12));
  framebuffer_width = mbi->framebuffer_width;
  framebuffer_height = mbi->framebuffer_height;

  uint32_t bits_pp = mbi->framebuffer_bpp;
  uint32_t bytes_pp = (bits_pp / 8) + (8 - (bits_pp % 8));

  framebuffer_size = bytes_pp * framebuffer_width * framebuffer_height;

  framebuffer_physical = mbi->framebuffer_addr;
  framebuffer =
      mmu_map_frames((void *)(uint32_t)mbi->framebuffer_addr, framebuffer_size);

  vbe_info.width = framebuffer_width;
  vbe_info.height = framebuffer_height;
  vbe_info.bpp = mbi->framebuffer_bpp;
}

vfs_vm_object_t *vbe_get_vm_object(uint64_t length, uint64_t offset,
                                   vfs_fd_t *fd) {
  (void)fd;
  (void)length;
  (void)offset;
  vbe_vm_object.size = framebuffer_size;
  int n = (uintptr_t)align_page((void *)(uint32_t)framebuffer_size) / 0x1000;
  vbe_vm_object.object = kmalloc(sizeof(void *) * n);
  for (int i = 0; i < n; i++) {
    vbe_vm_object.object[i] = (void *)framebuffer_physical + (i * 0x1000);
  }
  return &vbe_vm_object;
}

int display_info_read(uint8_t *buffer, uint64_t offset, uint64_t len,
                      vfs_fd_t *fd) {
  (void)offset;
  int read_len = min(sizeof(struct DISPLAY_INFO), len);
  memcpy(buffer, &vbe_info, read_len);
  return read_len;
}

void add_vbe_device(void) {
  devfs_add_file("/vbe", NULL, NULL, vbe_get_vm_object, 1, 1,
                 FS_TYPE_BLOCK_DEVICE);
  devfs_add_file("/display_info", display_info_read, NULL, NULL, 1, 0,
                 FS_TYPE_BLOCK_DEVICE);
}
