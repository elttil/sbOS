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
    *pointer = 0;
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
  u8 read_ptr = inb(nabm.address + 0x10 + 0x4);

  u8 buffer_left;
  if (entry >= read_ptr) {
    buffer_left = 32 - entry;
    if (0 == read_ptr && buffer_left > 0) {
      buffer_left--;
    }
  } else {
    buffer_left = read_ptr - entry - 1;
  }

  return (buffer_left > 0);
}

static int write_buffer(u8 *buffer, size_t size) {
  assert(size <= 128000);
  u8 read_ptr = inb(nabm.address + 0x10 + 0x4);

  u8 buffer_left;
  if (entry >= read_ptr) {
    buffer_left = 32 - entry;
    if (0 == read_ptr && buffer_left > 0) {
      buffer_left--;
    }
  } else {
    buffer_left = read_ptr - entry - 1;
  }

  if (0 == buffer_left) {
    return 0;
  }

  memset((u8 *)buffers[entry].data, 0, 128000);
  memcpy((u8 *)buffers[entry].data, buffer, size);

  outb(nabm.address + 0x10 + 0x5, entry);
  entry = (entry + 1) % 32;
  start();
  return 1;
}

char tmp_buffer[128000];
size_t tmp_buffer_count = 0;
int ac97_add_pcm(u8 *buffer, size_t len) {
  size_t wl = min(len, 128000 - tmp_buffer_count);
  memcpy(tmp_buffer + tmp_buffer_count, buffer, wl);
  tmp_buffer_count += wl;
  if (tmp_buffer_count < 128000) {
    return wl;
  }
  if (!write_buffer(tmp_buffer, 128000)) {
    return wl;
  }
  tmp_buffer_count = 0;
  return wl;
}

int ac97_current_volume = 100;
int ac97_get_volume(void) {
  return ac97_current_volume;
}

void ac97_set_volume(int volume) {
  assert(volume <= 100);
  ac97_current_volume = volume;
  int s;
  if (0 == volume) {
    s = 31;
  } else {
    s = (31 * volume) / 100;
  }

  u8 right_channel = 31 - s;
  u8 left_channel = 31 - s;

  // Set PCM Output Volume
  outw(nam.address + 0x18, right_channel | (left_channel << 8));
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

  outw(nam.address + 0x2C, 48000);
  outw(nam.address + 0x2E, 48000);
  outw(nam.address + 0x30, 48000);
  outw(nam.address + 0x32, 48000);

  /*
  As last thing, set maximal volume for
  PCM Output by writing value 0 to this register. Now sound card is
  ready to use.
  */
  ac97_set_volume(100);

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
