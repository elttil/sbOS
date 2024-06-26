#include <assert.h>
#include <errno.h>
#include <fs/shm.h>
#include <fs/vfs.h>
#include <hashmap/hashmap.h>
#include <mmu.h>
#include <sched/scheduler.h>
#include <stddef.h>

HashMap *shared_memory_objects;

void shm_init(void) {
  shared_memory_objects = hashmap_create(10);
}

int shm_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  vfs_vm_object_t *p = fd->inode->internal_object;

  if (offset > p->size) {
    return -EFBIG;
  }

  if (offset + len > p->size) {
    len = p->size - offset;
  }

  memcpy((void *)((uintptr_t)((uintptr_t)p->virtual_object + offset)), buffer,
         len);
  return len;
}

int shm_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  vfs_vm_object_t *p = fd->inode->internal_object;

  if (offset > p->size) {
    return -EFBIG;
  }

  if (offset + len > p->size) {
    len = p->size - offset;
  }

  memcpy((void *)buffer,
         (void *)((uintptr_t)((uintptr_t)p->virtual_object + offset)), len);
  return len;
}

vfs_vm_object_t *shm_get_vm_object(u64 length, u64 offset, vfs_fd_t *fd) {
  (void)length;
  (void)offset;
  vfs_vm_object_t *p = fd->inode->internal_object;
  return p;
}

int shm_ftruncate(vfs_fd_t *fd, size_t length) {
  vfs_vm_object_t *p = fd->inode->internal_object;
  if (p->real_pointer) {
    u8 *mem_region = p->real_pointer;
    mmu_free_address_range(mem_region, align_page(p->size), NULL);
  }

  p->real_pointer = mmu_find_unallocated_virtual_range(NULL, length);
  if (!mmu_allocate_region(p->real_pointer, length, MMU_FLAG_RW, NULL)) {
    return -ENOMEM;
  }
  p->size = length;

  p->virtual_object = p->real_pointer;

  int n = (uintptr_t)align_page((void *)(u32)length) / 0x1000;
  p->object = krealloc(p->object, sizeof(void *) * n);
  for (int i = 0; i < n; i++) {
    p->object[i] =
        (void *)(get_page(p->virtual_object + (i * 0x1000), NULL, 0, 0)->frame *
                 0x1000);
  }
  return 0;
}

int shm_open(const char *name, int oflag, mode_t mode) {
  // Try to find or create a new shared memory object.
  vfs_vm_object_t *internal_object =
      hashmap_get_entry(shared_memory_objects, name);
  if (!internal_object) {
    internal_object = kcalloc(1, sizeof(vfs_vm_object_t));
    hashmap_add_entry(shared_memory_objects, name, internal_object, NULL, 0);
  }

  vfs_inode_t *inode = vfs_create_inode(
      0 /*inode_num*/, 0 /*type*/, NULL, NULL, 1 /*is_open*/, internal_object,
      0 /*file_size*/, NULL /*open*/, NULL /*create_file*/, shm_read, shm_write,
      NULL /*close*/, NULL /*create_directory*/, shm_get_vm_object,
      shm_ftruncate, NULL /*stat*/, NULL /*send_signal*/, NULL /*connect*/);

  vfs_fd_t *fd_ptr;
  int fd = vfs_create_fd(oflag, mode, 0 /*is_tty*/, inode, &fd_ptr);
  if (-1 == fd) {
    kfree(inode);
    return -EMFILE;
  }
  return fd;
}

int shm_unlink(const char *name) {
  (void)name;
  return 0;
}
