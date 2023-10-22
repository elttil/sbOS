#include <assert.h>
#include <errno.h>
#include <fs/shm.h>
#include <fs/vfs.h>
#include <hashmap/hashmap.h>
#include <mmu.h>
#include <sched/scheduler.h>
#include <stddef.h>

HashMap *shared_memory_objects;

void shm_init(void) { shared_memory_objects = hashmap_create(10); }

int shm_write(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd) {
  vfs_vm_object_t *p = fd->inode->internal_object;

  if (offset > p->size)
    return -EFBIG;

  if (offset + len > p->size)
    len = p->size - offset;

  memcpy((void *)((uintptr_t)((uintptr_t)p->virtual_object + offset)), buffer,
         len);
  return len;
}

int shm_read(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd) {
  vfs_vm_object_t *p = fd->inode->internal_object;

  if (offset > p->size)
    return -EFBIG;

  if (offset + len > p->size)
    len = p->size - offset;

  memcpy((void *)buffer,
         (void *)((uintptr_t)((uintptr_t)p->virtual_object + offset)), len);
  return len;
}

vfs_vm_object_t *shm_get_vm_object(uint64_t length, uint64_t offset,
                                   vfs_fd_t *fd) {
  (void)length;
  (void)offset;
  vfs_vm_object_t *p = fd->inode->internal_object;
  return p;
}

int shm_open(const char *name, int oflag, mode_t mode) {
  // Try to find or create a new shared memory object.
  vfs_vm_object_t *internal_object =
      hashmap_get_entry(shared_memory_objects, name);
  if (!internal_object) {
    //    if (!(oflag & O_CREAT))
    //      return -EMFILE;
    internal_object = kmalloc(sizeof(vfs_vm_object_t));
    internal_object->object = NULL;
    internal_object->size = 0;
    hashmap_add_entry(shared_memory_objects, name, internal_object, NULL, 0);
  }

  vfs_inode_t *inode =
      vfs_create_inode(0 /*inode_num*/, 0 /*type*/, 1 /*has_data*/,
                       1 /*can_write*/, 1 /*is_open*/, internal_object,
                       0 /*file_size*/, NULL /*open*/, NULL /*create_file*/,
                       shm_read, shm_write, NULL /*close*/, shm_get_vm_object);

  vfs_fd_t *fd_ptr;
  int fd = vfs_create_fd(oflag, mode, inode, &fd_ptr);
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

int ftruncate(int fildes, uint64_t length) {
  kprintf("ftruncate: %d\n", length);
  vfs_fd_t *fd = get_current_task()->file_descriptors[fildes];
  if (!fd)
    return -EBADF;
  vfs_vm_object_t *p = fd->inode->internal_object;
  p->size = length;
  p->virtual_object = ksbrk(length);
  int n = (uintptr_t)align_page((void *)(uint32_t)length) / 0x1000;
  p->object = kmalloc(sizeof(void *) * n);
  for (int i = 0; i < n; i++)
    p->object[i] =
        (void *)(get_page(p->virtual_object + (i * 0x1000), NULL, 0, 0)->frame *
                 0x1000);
  return 0;
}
