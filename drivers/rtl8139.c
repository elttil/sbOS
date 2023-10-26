#include <assert.h>
#include <cpu/idt.h>
#include <cpu/io.h>
#include <drivers/pci.h>
#include <drivers/rtl8139.h>
#include <mmu.h>
#include <network/ethernet.h>

#define RBSTART 0x30
#define CMD 0x37
#define IMR 0x3C

#define TSD0 0x10  // transmit status
#define TSAD0 0x20 // transmit start address

struct PCI_DEVICE rtl8139;
uint8_t *device_buffer;

uint8_t *send_buffers[4];

struct _INT_PACKET_HEADER {
  uint8_t ROK : 1;
  uint8_t FAE : 1;
  uint8_t CRC : 1;
  uint8_t LONG : 1;
  uint8_t RUNT : 1;
  uint8_t ISE : 1;
  uint8_t reserved : 5;
  uint8_t BAR : 1;
  uint8_t PAM : 1;
  uint8_t MAR : 1;
};

struct PACKET_HEADER {
  union {
    uint16_t raw;
    struct _INT_PACKET_HEADER data;
  };
};

uint16_t current_packet_read = 0;

void handle_packet(void) {
  assert(sizeof(struct _INT_PACKET_HEADER) == sizeof(uint16_t));

  uint16_t *buf = (uint16_t *)(device_buffer + current_packet_read);
  struct PACKET_HEADER packet_header;
  packet_header.raw = *buf;
  assert(packet_header.data.ROK);
  kprintf("packet_header.raw: %x\n", packet_header.raw);
  uint16_t packet_length = *(buf + 1);
  kprintf("packet_length: %x\n", packet_length);

  uint8_t packet_buffer[8192 + 16];
  if (current_packet_read + packet_length >= 8192 + 16) {
    uint32_t first_run = ((uint8_t *)buf + (8192 + 16)) - device_buffer;
    memcpy(packet_buffer, buf, first_run);
    memcpy(packet_buffer, device_buffer, packet_length - first_run);
  } else {
    memcpy(packet_buffer, buf, packet_length);
  }

  handle_ethernet((uint8_t *)packet_buffer + 4, packet_length);

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
  uint16_t status = inw(rtl8139.gen.base_mem_io + 0x3e);
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

int rtl8139_send_data(uint8_t *data, uint16_t data_size) {
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
       (uint32_t)virtual_to_physical(send_buffers[loop], NULL));
  outl(device->gen.base_mem_io + 0x10 + loop * 4, data_size);
  loop += 1;
  return 1;
}

void get_mac_address(uint8_t mac[6]) {
  uint32_t base_address = rtl8139.gen.base_mem_io;
  // Read the MAC address
  uint64_t mac_address;
  {
    uint32_t low_mac = inl(base_address);
    uint16_t high_mac = inw(base_address + 0x4);
    mac_address = ((uint64_t)high_mac << 32) | low_mac;
  }
  kprintf("mac_address: %x\n", mac_address);
  memcpy(mac, &mac_address, sizeof(uint8_t[6]));
}

uint8_t rtl8139_get_transmit_status(uint32_t base_address) {
  uint32_t status_register = inl(base_address + 0x3E);
  if ((status_register >> 3) & 0x1)
    kprintf("transmit error :(\n");
  uint8_t status = (status_register >> 2) & 0x1;
  outl(base_address + 0x3E, 0x5);
  return status;
}

void rtl8139_init(void) {
  if (!pci_populate_device_struct(0x10EC, 0x8139, &rtl8139)) {
    kprintf("RTL8139 not found :(\n");
    return;
  }
  kprintf("RTL8139 found at bus: %x slot: %x\n", rtl8139.bus, rtl8139.slot);

  uint8_t header_type = (pci_config_read32(&rtl8139, 0, 0xC) >> 16) & 0xFF;
  assert(0 == header_type);

  uint32_t base_address = rtl8139.gen.base_mem_io;
  uint8_t interrupt_line = pci_get_interrupt_line(&rtl8139);

  // Enable bus mastering
  uint32_t register1 = pci_config_read32(&rtl8139, 0, 0x4);
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
  uint32_t rx_buffer = (uint32_t)virtual_to_physical(device_buffer, NULL);
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
  asm("sti");
  for (;;)
    asm("sti");
}
