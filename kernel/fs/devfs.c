#include <assert.h>
#include <drivers/keyboard.h>
#include <drivers/serial.h>
#include <fs/devfs.h>
#include <fs/vfs.h>
#include <random.h>

devfs_file files[20];
int num_files = 0;

vfs_inode_t *devfs_add_file(
    char *path, int (*read)(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd),
    int (*write)(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd),
    vfs_vm_object_t *(*get_vm_object)(u64 length, u64 offset, vfs_fd_t *fd),
    int (*has_data)(vfs_inode_t *inode), int (*can_write)(vfs_inode_t *inode),
    int type) {
  files[num_files].name = copy_and_allocate_string(path);

  vfs_inode_t *i = kcalloc(1, sizeof(vfs_inode_t));
  files[num_files].inode = i;
  i->type = type;
  i->read = read;
  i->write = write;
  i->close = NULL;
  i->get_vm_object = get_vm_object;
  i->_has_data = has_data;
  i->_can_write = can_write;
  i->is_open = 1;
  num_files++;
  return i;
}

vfs_inode_t *devfs_open(const char *file) {
  for (int i = 0; i < num_files; i++) {
    if (isequal_n(files[i].name, file, strlen(files[i].name))) {
      // FIXME: Temporary hack
      vfs_inode_t *r = kmalloc(sizeof(vfs_inode_t));
      if (!r) {
        return NULL; // TODO: Give some error condition
      }
      memcpy(r, files[i].inode, sizeof(vfs_inode_t));
      return r;
    }
  }

  return NULL;
}

int devfs_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  return fd->inode->read(buffer, offset, len, fd);
}

int devfs_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  return fd->inode->write(buffer, offset, len, fd);
}

vfs_vm_object_t *devfs_get_vm_object(u64 length, u64 offset, vfs_fd_t *fd) {
  return fd->inode->get_vm_object(length, offset, fd);
}

int stdout_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  (void)offset;
  (void)fd;

  int rc = len;
  for (; len--;) {
    write_serial(*buffer++);
  }
  return rc;
}

int serial_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  (void)offset;
  (void)fd;

  int rc = len;
  for (; len--;) {
    write_serial(*buffer++);
  }
  return rc;
}

int always_has_data(vfs_inode_t *inode) {
  (void)inode;
  return 1;
}

int always_can_write(vfs_inode_t *inode) {
  (void)inode;
  return 1;
}

void add_serial(void) {
  devfs_add_file("/serial", NULL, serial_write, NULL, NULL, always_can_write,
                 FS_TYPE_CHAR_DEVICE);
}

void add_stdout(void) {
  devfs_add_file("/stdout", NULL, stdout_write, NULL, NULL, always_can_write,
                 FS_TYPE_CHAR_DEVICE);
}

vfs_inode_t *devfs_mount(void) {
  vfs_inode_t *root = kcalloc(1, sizeof(vfs_inode_t));
  root->open = devfs_open;
  root->read = devfs_read;
  root->write = devfs_write;
  root->close = NULL;
  return root;
}
