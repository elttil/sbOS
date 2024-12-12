#include <cpu/idt.h>
#include <drivers/mouse.h>
#include <errno.h>
#include <fs/devfs.h>
#include <fs/fifo.h>
#include <fs/vfs.h>
#include <interrupts.h>
#include <math.h>
#include <typedefs.h>

u8 mouse_cycle = 0;
u8 mouse_u8[3];
int mouse_x = 0;
int mouse_y = 0;

u8 mouse_left_button = 0;
u8 mouse_middle_button = 0;
u8 mouse_right_button = 0;

vfs_inode_t *mouse_inode = NULL;

struct mouse_event {
  u8 buttons;
  int x;
  int y;
};

struct mouse_event previous_event = {
    .buttons = 0,
    .x = 0,
    .y = 0,
};

int fs_mouse_has_data(vfs_inode_t *inode) {
  if (mouse_x != previous_event.x || mouse_y != previous_event.y) {
    return 1;
  }

  const struct ringbuffer *rb = inode->internal_object;
  return !ringbuffer_isempty(rb);
}

int fs_mouse_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  mouse_middle_button = mouse_u8[0] & (1 << 2);
  mouse_right_button = mouse_u8[0] & (1 << 1);
  mouse_left_button = mouse_u8[0] & (1 << 0);

  struct ringbuffer *rb = fd->inode->internal_object;
  u32 rc = ringbuffer_read(rb, buffer, len);
  if (0 == rc && len > 0) {
    if (mouse_x != previous_event.x || mouse_y != previous_event.y) {
      int read_len = min(len, sizeof(struct mouse_event));

      struct mouse_event e;
      e.x = mouse_x;
      e.y = mouse_y;
      e.buttons = 0;
      if (mouse_middle_button) {
        e.buttons |= (1 << 2);
      }
      if (mouse_right_button) {
        e.buttons |= (1 << 1);
      }
      if (mouse_left_button) {
        e.buttons |= (1 << 0);
      }
      memcpy(buffer, &e, read_len);
      previous_event = e;
      return read_len;
    }
    return -EAGAIN;
  }
  return rc;
}

void add_mouse(void) {
  mouse_inode =
      devfs_add_file("/mouse", fs_mouse_read, NULL, NULL, fs_mouse_has_data,
                     always_can_write, FS_TYPE_CHAR_DEVICE);
  mouse_inode->internal_object = kmalloc(sizeof(struct ringbuffer));
  ringbuffer_init(mouse_inode->internal_object, 4096);
}

void int_mouse(reg_t *r) {
  (void)r;
  switch (mouse_cycle) {
  case 0:
    mouse_u8[0] = inb(0x60);
    if (!(mouse_u8[0] & (1 << 3))) {
      mouse_cycle = 0;
      break;
    }
    mouse_cycle++;
    break;
  case 1:
    mouse_u8[1] = inb(0x60);
    mouse_cycle++;
    break;
  case 2: {
    mouse_u8[2] = inb(0x60);
    int16_t x = mouse_u8[1];
    int16_t y = mouse_u8[2];
    uint8_t xs = mouse_u8[0] & (1 << 4);
    uint8_t ys = mouse_u8[0] & (1 << 5);
    if (xs) {
      x |= 0xFF00;
    }
    if (ys) {
      y |= 0xFF00;
    }

    mouse_x += x;
    mouse_y -= y;

    if (mouse_x < 0) {
      mouse_x = 0;
    }
    if (mouse_y < 0) {
      mouse_y = 0;
    }

    mouse_middle_button = mouse_u8[0] & (1 << 2);
    mouse_right_button = mouse_u8[0] & (1 << 1);
    mouse_left_button = mouse_u8[0] & (1 << 0);

    mouse_cycle = 0;

    // If there is a change in the mouse buttons that event should be written.
    // The mouse movements will be generated upon request. This avoids
    // filling the ringbuffer with a ton of small mouse movements.

    int button_change = 0;
    if (mouse_middle_button != (previous_event.buttons & (1 << 2))) {
      button_change = 1;
    } else if (mouse_right_button != (previous_event.buttons & (1 << 1))) {
      button_change = 1;
    } else if (mouse_left_button != (previous_event.buttons & (1 << 0))) {
      button_change = 1;
    }

    if (!button_change) {
      break;
    }

    struct mouse_event e;
    e.buttons = mouse_u8[0];
    e.x = mouse_x;
    e.y = mouse_y;
    previous_event = e;
    if (mouse_inode) {
      struct ringbuffer *rb = mouse_inode->internal_object;
      ringbuffer_write(rb, (u8 *)&e, sizeof(e));
    }
    break;
  }
  }
  EOI(0xC);
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

  install_handler((interrupt_handler)int_mouse, INT_32_INTERRUPT_GATE(0x3),
                  0x2C);
  irq_clear_mask(0x2);
}
