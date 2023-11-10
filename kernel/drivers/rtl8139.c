#include <assert.h>
#include <cpu/idt.h>
#include <cpu/io.h>
#include <drivers/pci.h>
#include <drivers/rtl8139.h>
#include <mmu.h>
#include <network/arp.h>
#include <network/ethernet.h>

#define RBSTART 0x30
#define CMD 0x37
#define IMR 0x3C

#define TSD0 0x10  // transmit status
#define TSAD0 0x20 // transmit start address

struct PCI_DEVICE rtl8139;
u8 *device_buffer;

u8 *send_buffers[4];

struct _INT_PACKET_HEADER {
  u8 ROK : 1;
  u8 FAE : 1;
  u8 CRC : 1;
  u8 LONG : 1;
  u8 RUNT : 1;
  u8 ISE : 1;
  u8 reserved : 5;
  u8 BAR : 1;
  u8 PAM : 1;
  u8 MAR : 1;
};

struct PACKET_HEADER {
  union {
    u16 raw;
    struct _INT_PACKET_HEADER data;
  };
};

u16 current_packet_read = 0;

void handle_packet(void) {
  assert(sizeof(struct _INT_PACKET_HEADER) == sizeof(u16));

  u16 *buf = (u16 *)(device_buffer + current_packet_read);
  struct PACKET_HEADER packet_header;
  packet_header.raw = *buf;
  assert(packet_header.data.ROK);
  kprintf("packet_header.raw: %x\n", packet_header.raw);
  u16 packet_length = *(buf + 1);
  kprintf("packet_length: %x\n", packet_length);

  u8 packet_buffer[8192 + 16];
  if (current_packet_read + packet_length >= 8192 + 16) {
    u32 first_run = ((u8 *)buf + (8192 + 16)) - device_buffer;
    memcpy(packet_buffer, buf, first_run);
    memcpy(packet_buffer, device_buffer, packet_length - first_run);
  } else {
    memcpy(packet_buffer, buf, packet_length);
  }

  handle_ethernet((u8 *)packet_buffer + 4, packet_length);

  // Thanks to exscape
  // https://github.com/exscape/exscapeOS/blob/master/src/kernel/net/rtl8139.c
  // and the programmers guide
  // https://www.cs.usfca.edu/~cruse/cs326f04/RTL8139_ProgrammersGuide.pdf I
  // have no clue what this calculation, I can't find anything possibly relating
  // to this in the manual, but it does work I guess.
  current_packet_read = (current_packet_read + packet_length + 4 + 3) & (~3);
  current_packet_read %= 8192 + 16;
  outw(rtl8139.gen.base_mem_io + 0x38, current_packet_read - 0x10);
}

__attribute__((interrupt)) void rtl8139_handler(void *regs) {
  (void)regs;
  u16 status = inw(rtl8139.gen.base_mem_io + 0x3e);
  kprintf("status: %x\n", status);

  outw(rtl8139.gen.base_mem_io + 0x3E, 0x5);
  if (status & (1 << 2)) {
    kprintf("Packet sent\n");
  }
  if (status & (1 << 0)) {
    kprintf("Received packet\n");
    handle_packet();
  }

  EOI(0xB);
}

int rtl8139_send_data(u8 *data, u16 data_size) {
  const struct PCI_DEVICE *device = &rtl8139;
  // FIXME: It should block or fail if there is too little space for the
  // buffer
  if (data_size > 0x1000)
    return 0;
  static int loop = 0;
  if (loop > 3) {
    loop = 0;
  }
  memcpy(send_buffers[loop], data, data_size);
  outl(device->gen.base_mem_io + 0x20 + loop * 4,
       (u32)virtual_to_physical(send_buffers[loop], NULL));
  outl(device->gen.base_mem_io + 0x10 + loop * 4, data_size);
  loop += 1;
  return 1;
}

void get_mac_address(u8 mac[6]) {
  u32 base_address = rtl8139.gen.base_mem_io;
  // Read the MAC address
  u64 mac_address;
  {
    u32 low_mac = inl(base_address);
    u16 high_mac = inw(base_address + 0x4);
    mac_address = ((u64)high_mac << 32) | low_mac;
  }
  kprintf("mac_address: %x\n", mac_address);
  memcpy(mac, &mac_address, sizeof(u8[6]));
}

u8 rtl8139_get_transmit_status(u32 base_address) {
  u32 status_register = inl(base_address + 0x3E);
  if ((status_register >> 3) & 0x1)
    kprintf("transmit error :(\n");
  u8 status = (status_register >> 2) & 0x1;
  outl(base_address + 0x3E, 0x5);
  return status;
}

void rtl8139_init(void) {
  if (!pci_populate_device_struct(0x10EC, 0x8139, &rtl8139)) {
    kprintf("RTL8139 not found :(\n");
    return;
  }
  kprintf("RTL8139 found at bus: %x slot: %x\n", rtl8139.bus, rtl8139.slot);

  u8 header_type = (pci_config_read32(&rtl8139, 0, 0xC) >> 16) & 0xFF;
  assert(0 == header_type);

  u32 base_address = rtl8139.gen.base_mem_io;
  u8 interrupt_line = pci_get_interrupt_line(&rtl8139);

  // Enable bus mastering
  u32 register1 = pci_config_read32(&rtl8139, 0, 0x4);
  register1 |= (1 << 2);
  pci_config_write32(&rtl8139, 0, 0x4, register1);

  // Turning on the device
  outb(base_address + 0x52, 0x0);

  // Reset the device and clear the RX and TX buffers
  outb(base_address + CMD, 0x10);
  for (; 0 != (inb(base_address + CMD) & 0x10);)
    ;
  device_buffer = ksbrk(8192 + 16);
  memset(device_buffer, 0, 8192 + 16);
  // Setupt the recieve buffer
  u32 rx_buffer = (u32)virtual_to_physical(device_buffer, NULL);
  outl(base_address + RBSTART, rx_buffer);

  // Set IMR + ISR
  outw(base_address + IMR, (1 << 2) | (1 << 3) | (1 << 0));

  // Set transmit and reciever enable
  outb(base_address + 0x37, (1 << 2) | (1 << 3));

  // Configure the recieve buffer
  outl(base_address + 0x44,
       0xf); // 0xf is AB+AM+APM+AAP

  install_handler(rtl8139_handler, INT_32_INTERRUPT_GATE(0x0),
                  0x20 + interrupt_line);

  // ksbrk() seems to have the magical ability of disabling interrupts?
  // I have no fucking clue why that happens and it was a pain to debug.
  for (int i = 0; i < 4; i++)
    send_buffers[i] = ksbrk(0x1000);
}
