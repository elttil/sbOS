#include <assert.h>
#include <cpu/io.h>
#include <drivers/ac97.h>
#include <drivers/pci.h>
#include <fcntl.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <math.h>
#include <random.h>

struct PCI_DEVICE ac97;
struct PCI_BaseAddressRegister nabm;
struct PCI_BaseAddressRegister nam;

int current_pointer = 0;

void *physical_buffer_descriptor_list;
u16 *buffer_descriptor_list;
u16 *pointer;

struct audio_buffer {
  volatile u8 *data;
  size_t data_written;
  uintptr_t physical;
  int has_played;
};

struct audio_buffer buffers[32];

void add_buffer(void) {
  u16 *pointer = buffer_descriptor_list;
  for (int i = 0; i < 32; i++) {
    void *physical_audio_data;
    u8 *audio_data = kmalloc_align(128000, &physical_audio_data);
    assert(audio_data);

    buffers[i].data = audio_data;
    buffers[i].physical = physical_audio_data;
    buffers[i].has_played = 1;
    buffers[i].data_written = 0;

    *((u32 *)pointer) = physical_audio_data;
    pointer += 2;
    *pointer = 128000 / 2;
    pointer++;
    *pointer = (1 << 14);
    pointer++;
  }
}

void add_to_list(u8 *buffer, size_t size) {
  void *physical_audio_data;
  u8 *audio_data = kmalloc_align(size, &physical_audio_data);
  memcpy(audio_data, buffer, size);

  // Write number of last valid buffer entry to Last Valid Entry
  // register (NABM register 0x15)
  outb(nabm.address + 0x10 + 0x5, 1);

  // Set bit for transfering data (NABM register 0x1B, value 0x1)
  u8 s = inb(nabm.address + 0x10 + 0xB);
  s |= (1 << 0);
  outb(nabm.address + 0x10 + 0xB, s);

  outl(nabm.address + 0x10 + 0x0, physical_buffer_descriptor_list);
}

int entry = 0;

void start(void) {
  // Set bit for transfering data (NABM register 0x1B, value 0x1)
  u8 s = inb(nabm.address + 0x10 + 0xB);
  if (s & (1 << 0)) {
    return;
  }
  s |= (1 << 0);
  outb(nabm.address + 0x10 + 0xB, s);
}

int ac97_can_write(void) {
  u8 process_num = inb(nabm.address + 0x10 + 0x4);
  if (!buffers[entry].has_played) {
    if (31 == process_num) {
      outb(nabm.address + 0x10 + 0x5, entry);
      buffers[31].has_played = 1;
    }
    for (int i = 0; i < process_num; i++) {
      buffers[i].has_played = 1;
    }
  }
  u8 delta = abs((int)process_num - (int)entry);
  if (delta > 3) {
    return 0;
  }
  return buffers[entry].has_played;
}

static int write_buffer(u8 *buffer, size_t size) {
  u8 process_num = inb(nabm.address + 0x10 + 0x4);
  if (!buffers[entry].has_played && 128000 == buffers[entry].data_written) {
    if (31 == process_num) {
      outb(nabm.address + 0x10 + 0x5, entry);
      buffers[31].has_played = 1;
    }
    for (int i = 0; i < process_num; i++) {
      buffers[i].has_played = 1;
    }
    if (!buffers[entry].has_played) {
      return 0;
    }
  }

  u8 delta = abs((int)process_num - (int)entry);
  if (delta > 3) {
    return 0;
  }

  size_t offset = buffers[entry].data_written;
  memset((u8 *)buffers[entry].data + offset, 0, 128000 - offset);
  size_t wl = min(size, 128000 - offset);
  memcpy((u8 *)buffers[entry].data + offset, buffer, wl);
  buffers[entry].data_written += wl;
  buffers[entry].has_played = 0;

  if (128000 == buffers[entry].data_written) {
    u8 current_valid = inb(nabm.address + 0x10 + 0x5);
    if (current_valid <= entry) {
      outb(nabm.address + 0x10 + 0x5, entry);
    }
    entry++;
    if (entry > 31) {
      entry = 0;
    }
  }

  start();
  return 1;
}

int ac97_add_pcm(u8 *buffer, size_t len) {
  size_t rc = 0;
  for (; len > 0;) {
    size_t wl = min(len, 128000);
    if (!write_buffer(buffer + rc, wl)) {
      break;
    }
    rc += wl;
    len -= wl;
  }
  return rc;
}

void ac97_init(void) {
  if (!pci_populate_device_struct(0x8086, 0x2415, &ac97)) {
    assert(0);
    return;
  }

  pointer = buffer_descriptor_list =
      kmalloc_align(0x1000, &physical_buffer_descriptor_list);

  // Enable bus mastering
  u32 register1 = pci_config_read32(&ac97, 0, 0x4);
  register1 |= (1 << 0);
  register1 |= (1 << 2);
  pci_config_write32(&ac97, 0, 0x4, register1);

  assert(pci_get_bar(&ac97, 1, &nabm));
  assert(nabm.type == PCI_BAR_IO);

  assert(pci_get_bar(&ac97, 0, &nam));
  assert(nam.type == PCI_BAR_IO);

  /*
  In initalization of sound card you must resume card from cold reset
  and set power for it. It can be done by writing value 0x2 to Global
  Control register if you do not want to use interrupts, or 0x3 if you
  want to use interrupts.
  */

  outl(nabm.address + 0x2C, (1 << 1));

  /*
  After this, you should write any value to NAM
  Reset register to reset all NAM registers.
  */

  outw(nam.address + 0x0, 0x1);

  /*
  After this, you can read
  card capability info from Global Status register to found out if 20
  bit audio samples are supported and also check out bit 4 in NAM
  Capabilites register and value in AUX Output to find out if this sound
  card support headhone output.
  */

  outw(nam.address + 0x2C, 32000);
  outw(nam.address + 0x2E, 32000);
  outw(nam.address + 0x30, 32000);
  outw(nam.address + 0x32, 32000);

  /*
  As last thing, set maximal volume for
  PCM Output by writing value 0 to this register. Now sound card is
  ready to use.
  */

  // Set PCM Output Volume
  outw(nam.address + 0x18, 0);

  // Playing sound
  /*
    Set volume of output you want to use - Master Volume register (NAM
    register 0x02) for speaker and if supported, AUX Output register (NAM
    register 0x04) for headphone
    */
  // FIXME: Speaker but is this correct?
  outw(nam.address + 0x02, 0);
  outw(nam.address + 0x04, 0);

  // Set reset bit of output channel (NABM register 0x1B, value 0x2) and
  // wait for card to clear it
  u8 s = inb(nabm.address + 0x10 + 0xB);
  s |= (1 << 1);
  outb(nabm.address + 0x10 + 0xB, s);

  for (; inb(nabm.address + 0x10 + 0xB) & (1 << 1);)
    ;

  add_buffer();

  // Write physical position of BDL to Buffer Descriptor Base Address
  // register (NABM register 0x10)
  outl(nabm.address + 0x10 + 0x0, physical_buffer_descriptor_list);
}
