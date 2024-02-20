#include <cpu/idt.h>
#include <drivers/mouse.h>
#include <fs/devfs.h>
#include <fs/fifo.h>
#include <fs/vfs.h>
#include <interrupts.h>
#include <typedefs.h>

u8 mouse_cycle = 0;
u8 mouse_u8[3];
u8 mouse_x = 0;
u8 mouse_y = 0;
vfs_inode_t *mouse_inode;
vfs_fd_t *mouse_fd;

struct mouse_event {
  u8 buttons;
  u8 x;
  u8 y;
};

int fs_mouse_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  int rc = fifo_object_write(buffer, offset, len, fd->inode->internal_object);
  FIFO_FILE *f = fd->inode->internal_object;
  mouse_inode->has_data = f->has_data;
  return rc;
}

int fs_mouse_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  FIFO_FILE *f = fd->inode->internal_object;
  if (!mouse_inode->has_data) {
    return 0;
  }
  int rc = fifo_object_read(buffer, offset, len, f);
  mouse_inode->has_data = f->has_data;
  return rc;
}

void add_mouse(void) {
  mouse_inode = devfs_add_file("/mouse", fs_mouse_read, fs_mouse_write, NULL, 0,
                               0, FS_TYPE_CHAR_DEVICE);
  mouse_inode->internal_object = create_fifo_object();
  // Don't look at this
  int fd = vfs_open("/dev/mouse", O_RDWR, 0);
  mouse_fd = get_vfs_fd(fd);
  get_current_task()->file_descriptors[fd] = NULL;
}

void what(registers_t *r) {
  EOI(0xe);
}

void int_mouse(reg_t *r) {
  (void)r;
  EOI(12);
  switch (mouse_cycle) {
  case 0:
    mouse_u8[0] = inb(0x60);
    if (!(mouse_u8[0] & (1 << 3))) {
      mouse_cycle = 0;
      return;
    }
    mouse_cycle++;
    break;
  case 1:
    mouse_u8[1] = inb(0x60);
    mouse_cycle++;
    break;
  case 2:
    mouse_u8[2] = inb(0x60);
    mouse_x = mouse_u8[1];
    mouse_y = mouse_u8[2];
    mouse_cycle = 0;
    struct mouse_event e;
    e.buttons = mouse_u8[0];
    e.x = mouse_x;
    e.y = mouse_y;
    raw_vfs_pwrite(mouse_fd, &e, sizeof(e), 0);
    break;
  }
}

void mouse_wait(u8 a_type) {
  u32 _time_out = 100000;
  if (a_type == 0) {
    while (_time_out--) {
      if ((inb(0x64) & 1) == 1) {
        return;
      }
    }
    return;
  } else {
    while (_time_out--) {
      if ((inb(0x64) & 2) == 0) {
        return;
      }
    }
    return;
  }
}

void mouse_write(u8 a_write) {
  // Wait to be able to send a command
  mouse_wait(1);
  // Tell the mouse we are sending a command
  outb(0x64, 0xD4);
  // Wait for the final part
  mouse_wait(1);
  // Finally write
  outb(0x60, a_write);
}

u8 mouse_read() {
  mouse_wait(0);
  return inb(0x60);
}

void install_mouse(void) {
  u8 _status;
  disable_interrupts();
  // Enable the auxiliary mouse device
  mouse_wait(1);
  outb(0x64, 0xA8);

  // Enable the interrupts
  mouse_wait(1);
  outb(0x64, 0x20);
  mouse_wait(0);
  _status = (inb(0x60) | 2);
  mouse_wait(1);
  outb(0x64, 0x60);
  mouse_wait(1);
  outb(0x60, _status);

  // Tell the mouse to use default settings
  mouse_write(0xF6);
  mouse_read(); // Acknowledge

  // Enable the mouse
  mouse_write(0xF4);
  mouse_read(); // Acknowledge

  install_handler(int_mouse, INT_32_INTERRUPT_GATE(0x3), 12 + 0x20);
  install_handler(what, INT_32_INTERRUPT_GATE(0x3), 0xe + 0x20);
}
