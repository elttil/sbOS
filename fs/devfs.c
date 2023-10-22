#include <drivers/keyboard.h>
#include <drivers/serial.h>
#include <fs/devfs.h>
#include <fs/vfs.h>
#include <random.h>

devfs_file files[20];
int num_files = 0;

vfs_inode_t *devfs_add_file(
    char *path,
    int (*read)(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd),
    int (*write)(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd),
    vfs_vm_object_t *(get_vm_object)(uint64_t length, uint64_t offset,
                                     vfs_fd_t *fd),
    uint8_t has_data, uint8_t can_write, int type) {
  files[num_files].name = copy_and_allocate_string(path);

  vfs_inode_t *i = kmalloc(sizeof(vfs_inode_t));
  files[num_files].inode = i;
  i->type = type;
  i->read = read;
  i->write = write;
  i->close = NULL;
  i->get_vm_object = get_vm_object;
  i->has_data = has_data;
  i->is_open = 1;
  i->can_write = can_write;
  num_files++;
  return i;
}

vfs_inode_t *devfs_open(const char *file) {
  for (int i = 0; i < num_files; i++)
    if (isequal_n(files[i].name, file, strlen(files[i].name)))
      return files[i].inode;

  return 0;
}

int devfs_read(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd) {
  return fd->inode->read(buffer, offset, len, fd);
}

int devfs_write(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd) {
  return fd->inode->write(buffer, offset, len, fd);
}

vfs_vm_object_t *devfs_get_vm_object(uint64_t length, uint64_t offset,
                                     vfs_fd_t *fd) {
  return fd->inode->get_vm_object(length, offset, fd);
}

int stdout_write(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd) {
  (void)offset;
  (void)fd;

  int rc = len;
  for (; len--;)
    putc(*buffer++);
  return rc;
}

int serial_write(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd) {
  (void)offset;
  (void)fd;

  int rc = len;
  for (; len--;)
    write_serial(*buffer++);
  return rc;
}

void add_serial(void) {
  devfs_add_file("/serial", NULL, serial_write, NULL, 0, 1,
                 FS_TYPE_CHAR_DEVICE);
}

void add_stdout(void) {
  devfs_add_file("/stdout", NULL, stdout_write, NULL, 0, 1,
                 FS_TYPE_CHAR_DEVICE);
}

vfs_inode_t *devfs_mount(void) {
  vfs_inode_t *root = kmalloc_eternal(sizeof(vfs_inode_t));
  root->open = devfs_open;
  root->read = devfs_read;
  root->write = devfs_write;
  root->close = NULL;
  return root;
}
