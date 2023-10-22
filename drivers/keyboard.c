#include <assert.h>
#include <drivers/keyboard.h>
#include <errno.h>
#include <fs/devfs.h>
#include <fs/fifo.h>
#include <fs/vfs.h>
#include <sched/scheduler.h>
#include <stdint.h>

#define PS2_REG_DATA 0x60
#define PS2_REG_STATUS 0x64
#define PS2_REG_COMMAND 0x64

#define PS2_CMD_ENABLE_FIRST_PORT 0xAE // no rsp

#define PS2_CMD_TEST_CONTROLLER 0xAA // has rsp

#define PS2_RSP_TEST_PASSED 0x55
#define PS2_RSP_TEST_FAILED 0xFC

#define PS2_CMD_SET_SCANCODE 0xF0 // has rsp
#define PS2_KB_ACK 0xFA
#define PS2_KB_RESEND 0xFE

#define PS2_CMD_SET_MAKE_RELEASE 0xF8 // has rsp

uint8_t kb_scancodes[3] = {0x43, 0x41, 0x3f};

FIFO_FILE *keyboard_fifo;

uint8_t ascii_table[] = {
    'e', '\x1B', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,
    '\t',

    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    //	0, // [
    //	0, // ]
    //	0,
    //	0, // ?
    '[', ']',
    '\n', // ENTER
    'C',

    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';',  // ;
    '\'', // ;
    '`',  // ;
    'D',  // LEFT SHIFT
    '\\', // ;
    'z', 'x', 'c', 'v', 'b', 'n', 'm',
    ',', // ;
    '.', // ;
    '/', // ;
    'U', // ;
    'U', // ;
    'U', // ;
    ' ', // ;
};

uint8_t capital_ascii_table[] = {
    'e', '\x1B', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8,
    '\t',

    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    //	0, // [
    //	0, // ]
    //	0,
    //	0, // ?
    '{', '}',
    '\n', // ENTER
    'C',

    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    ':',  // ;
    '\"', // ;
    '~',  // ;
    'D',  // LEFT SHIFT
    '\\', // ;
    'Z', 'X', 'C', 'V', 'B', 'N', 'M',
    '<', // ;
    '>', // ;
    '?', // ;
    'U', // ;
    'U', // ;
    'U', // ;
    ' ', // ;
};

vfs_inode_t *kb_inode;

uint8_t keyboard_to_ascii(uint16_t key, uint8_t capital) {
  if ((key & 0xFF) > sizeof(ascii_table))
    return 'U';
  if (capital)
    return capital_ascii_table[key & 0xFF];
  else
    return ascii_table[key & 0xFF];
}

uint8_t is_shift_down = 0;
uint8_t is_alt_down = 0;

struct KEY_EVENT {
  char c;
  uint8_t mode;    // (shift (0 bit)) (alt (1 bit))
  uint8_t release; // 0 pressed, 1 released
};

extern process_t *ready_queue;
__attribute__((interrupt)) void
int_keyboard(__attribute__((unused)) struct interrupt_frame *frame) {
  outb(0x20, 0x20);
  uint16_t c;
  c = inb(PS2_REG_DATA);
  int released = 0;
  if (c & 0x80) {
    switch ((c & ~(0x80)) & 0xFF) {
    case 0x2A: // Left shift
    case 0x36: // Right shift
      is_shift_down = 0;
      return;
    case 0x38:
      is_alt_down = 0;
      return;
    }
    released = 1;
  } else {
    switch (c & 0xFF) {
    case 0x2A: // Left shift
    case 0x36: // Right shift
      is_shift_down = 1;
      return;
    case 0x38:
      is_alt_down = 1;
      return;
    }
    released = 0;
  }
  unsigned char a = keyboard_to_ascii((c & ~(0x80)) & 0xFF, is_shift_down);
  struct KEY_EVENT ev;
  ev.c = a;
  ev.release = released;
  ev.mode = 0;
  ev.mode |= is_shift_down << 0;
  ev.mode |= is_alt_down << 1;
  fifo_object_write((uint8_t *)&ev, 0, sizeof(ev), keyboard_fifo);
  kb_inode->has_data = keyboard_fifo->has_data;
}

#define PS2_WAIT_RECV                                                          \
  {                                                                            \
    for (;;) {                                                                 \
      uint8_t status = inb(PS2_REG_STATUS);                                    \
      if (status & 0x1)                                                        \
        break;                                                                 \
    }                                                                          \
  }

#define PS2_WAIT_SEND                                                          \
  {                                                                            \
    for (;;) {                                                                 \
      uint8_t status = inb(PS2_REG_STATUS);                                    \
      if (!(status & (0x1 << 1)))                                              \
        break;                                                                 \
    }                                                                          \
  }

void install_keyboard(void) {
  keyboard_fifo = create_fifo_object();
  install_handler(int_keyboard, INT_32_INTERRUPT_GATE(0x3), 0x21);
}

int keyboard_read(uint8_t *buffer, uint64_t offset, uint64_t len,
                  vfs_fd_t *fd) {
  (void)offset;

  if (0 == fd->inode->has_data) {
    return -EAGAIN;
  }
  int rc = fifo_object_read(buffer, 0, len, keyboard_fifo);
  fd->inode->has_data = keyboard_fifo->has_data;
  return rc;
}

void add_keyboard(void) {
  kb_inode = devfs_add_file("/keyboard", keyboard_read, NULL, NULL, 0, 0,
                            FS_TYPE_CHAR_DEVICE);
}
