#include <assert.h>
#include <cpu/io.h>
#include <drivers/pci.h>
#include <drivers/rtl8139.h>
#include <mmu.h>

#define RBSTART 0x30
#define CMD 0x37
#define IMR 0x3C

struct PCI_DEVICE rtl8139;
uint8_t device_buffer[8192 + 16];

void rtl8139_init(void) {
  if (!pci_populate_device_struct(0x10EC, 0x8139, &rtl8139)) {
    kprintf("RTL8139 not found :(\n");
    return;
  }
  kprintf("RTL8139 found at bus: %x slot: %x\n", rtl8139.bus, rtl8139.slot);

  uint8_t header_type = (pci_config_read32(&rtl8139, 0, 0xC) >> 16) & 0xFF;
  assert(0 == header_type);

  uint32_t base_address = pci_config_read32(&rtl8139, 0, 0x10);
  uint8_t interrupt_line = pci_config_read32(&rtl8139, 0, 0x3C);
  kprintf("interrupt_line: %x\n", interrupt_line);

  // Turning on the device
  outb(base_address + 0x52, 0x0);

  // Reset the device and clear the RX and TX buffers
  outb(base_address + CMD, 0x10);
  for (; 0 != (inb(base_address + CMD) & 0x10);)
    ;

  // Setupt the recieve buffer
  uint32_t rx_buffer = (uint32_t)virtual_to_physical(device_buffer, NULL);
  outl(base_address + RBSTART, rx_buffer);

  // Set IMR + ISR
  outw(base_address + IMR, 0x0005); // Sets the TOK and ROK bits high

  // Configure the recieve buffer
  outl(base_address + 0x44,
       0xf | (1 << 7)); // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP

  // Enable recieve and transmitter
  outb(base_address + 0x37, 0x0C); // Sets the RE and TE bits high
  asm("sti");
}
