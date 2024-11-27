#include <assert.h>
#include <cpu/io.h>
#include <drivers/ac97.h>
#include <drivers/pci.h>
#include <fcntl.h>
#include <fs/vfs.h>
#include <kmalloc.h>
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
  uintptr_t physical;
  int has_played;
};

struct audio_buffer buffers[32];

void add_buffer(void) {
  u16 *pointer = buffer_descriptor_list;
  for (int i = 0; i < 4; i++) {
    void *physical_audio_data;
    u8 *audio_data = kmalloc_align(128000, &physical_audio_data);

    buffers[i].data = audio_data;
    buffers[i].physical = physical_audio_data;
    buffers[i].has_played = 1;

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
  //  // Write number of last valid buffer entry to Last Valid Entry
  //  // register (NABM register 0x15)
  //  outb(nabm.address + 0x10 + 0x5, 1);

  // Set bit for transfering data (NABM register 0x1B, value 0x1)
  u8 s = inb(nabm.address + 0x10 + 0xB);
  if (s & (1 << 0)) {
    return;
  }
  s |= (1 << 0);
  outb(nabm.address + 0x10 + 0xB, s);
}

void play_pcm(u8 *buffer, size_t size) {
  if (entry >= 3) {
    outb(nabm.address + 0x10 + 0x5, entry - 1);
    return;
  }
  if (!buffers[entry].has_played) {
    for (;;) {
      u8 process_num = inb(nabm.address + 0x10 + 0x4);
      kprintf("process_num: %d\n", process_num);
      if (process_num > entry) {
        buffers[entry].has_played = 1;
        break;
      }
    }
  }
  memcpy((u8 *)buffers[entry].data, buffer, size);
  outb(nabm.address + 0x10 + 0x5, entry);
  buffers[entry].has_played = 0;

  entry++;

  /*
  u8 process_num = inb(nabm.address + 0x10 + 0x4);
  kprintf("process_num: %d\n", process_num);

  if (0 == process_num)
    return;
    */
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

  // 0x30 	Global Status Register 	dword
  u32 status = inl(nabm.address + 0x30);

  u8 channel_capabilities = (status >> 20) & 0x3;
  u8 sample_capabilities = (status >> 22) & 0x3;
  kprintf("channel: %d\n", channel_capabilities);
  kprintf("sample: %d\n", sample_capabilities);

  //  outw(nam.address + 0x2C, 16000 / 2);
  outw(nam.address + 0x2C, 48000 / 2);
  u16 sample_rate = inw(nam.address + 0x2C);
  kprintf("sample_rate: %d\n", sample_rate);
  sample_rate = inw(nam.address + 0x2E);
  kprintf("sample_rate: %d\n", sample_rate);
  sample_rate = inw(nam.address + 0x30);
  kprintf("sample_rate: %d\n", sample_rate);
  sample_rate = inw(nam.address + 0x32);
  kprintf("sample_rate: %d\n", sample_rate);

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

  // SETUP?

  // Set reset bit of output channel (NABM register 0x1B, value 0x2) and
  // wait for card to clear it
  u8 s = inb(nabm.address + 0x10 + 0xB);
  s |= (1 << 1);
  outb(nabm.address + 0x10 + 0xB, s);

  kprintf("wait for clear\n");
  for (; inb(nabm.address + 0x10 + 0xB) & (1 << 1);)
    ;
  kprintf("cleared\n");

  add_buffer();

  // Write physical position of BDL to Buffer Descriptor Base Address
  // register (NABM register 0x10)
  outl(nabm.address + 0x10 + 0x0, physical_buffer_descriptor_list);

  // END SETUP?

  int fd = vfs_open("/hq.pcm", O_RDONLY, 0);
  assert(0 >= fd);
  size_t offset = 0;
  size_t capacity = 128000;
  char *data = kmalloc(capacity);
  for (;;) {
    int rc = vfs_pread(fd, data, capacity, offset);
    if (0 == rc) {
      break;
    }
    if (rc < 0) {
      kprintf("rc: %d\n", rc);
      assert(0);
    }
    play_pcm(data, rc);
    start();
    offset += rc;
  }
  for (;;)
    ;
}
