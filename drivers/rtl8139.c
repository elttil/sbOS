#include <assert.h>
#include <cpu/idt.h>
#include <cpu/io.h>
#include <drivers/pci.h>
#include <drivers/rtl8139.h>
#include <mmu.h>

#define RBSTART 0x30
#define CMD 0x37
#define IMR 0x3C

#define TSD0 0x10  // transmit status
#define TSAD0 0x20 // transmit start address

struct PCI_DEVICE rtl8139;
uint8_t device_buffer[8192 + 16];

__attribute__((interrupt)) void test_handler(void *regs) {
  kprintf("RUNNING TEST HANDLER :D\n");
  for (;;)
    ;
}

uint8_t pci_get_interrupt_line(const struct PCI_DEVICE *device) {
  return pci_config_read32(device, 0, 0x3C) & 0xFF;
}

void pci_set_interrupt_line(const struct PCI_DEVICE *device, uint8_t line) {
  uint32_t reg = pci_config_read32(device, 0, 0x3C);
  reg &= ~(0xFF);
  reg |= line;
  pci_config_write32(device, 0, 0x3C, reg);
}

void rtl8139_send_transmit_size(
    uint32_t base_address /*specify device instead of base_address*/,
    uint32_t size) {
  // Sets the OWN flag to '1'
  uint32_t status_register = inl(base_address + 0x10);
  status_register &= ~(1 << 13);
  outl(base_address + 0x10, status_register);
}

void rtl8139_send_transmit_buffer(uint32_t base_address, uint16_t buffer_size) {
  uint32_t status_register = inl(base_address + 0x10);
  // The size of this packet
  status_register &= ~(0x1fff);
  status_register |= buffer_size;

  // the early transmit threshol
  status_register |= 1 << 16;

  // Sets the OWN flag to '0'
  status_register &= ~(1 << 13);
  outl(base_address + 0x10, status_register);
  // Check if the OWN flag is '1'
  for (; !((inl(base_address + 0x10) >> 13) & 1);)
    ;
  // Check if the TOK flag is '1'
  for (; !((inl(base_address + 0x10) >> 15) & 1);)
    ;
  kprintf("TOK(IMR): %x\n", (inw(base_address + 0x3C) >> 2) & 0x1);
  kprintf("TER(IMR): %x\n", (inw(base_address + 0x3C) >> 3) & 0x1);

  kprintf("TOK(ISR): %x\n", (inw(base_address + 0x3E) >> 2) & 0x1);
  kprintf("TER(ISR): %x\n", (inw(base_address + 0x3E) >> 3) & 0x1);
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

  uint32_t base_address = pci_config_read32(&rtl8139, 0, 0x10) & (~0x3);
  uint8_t interrupt_line = pci_get_interrupt_line(&rtl8139);
  kprintf("base_address: %x\n", base_address);
  kprintf("interrupt_line: %x\n", interrupt_line);

  // Read the MAC address
  kprintf("MAC: %x\n", inl(base_address));

  // Enable bus mastering
  //  uint32_t register1 = pci_config_read32(&rtl8139, 0, 0x4);
  //  register1 |= (1 << 2);
  //  pci_config_write32(&rtl8139, 0, 0x4, register1);

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
  outw(base_address + IMR, (1 << 2) | (1 << 3));

  // Enable recieve and transmitter
  outb(base_address + 0x37,
       (1 << 2) | (1 << 3)); // Sets the RE and TE bits high

  // Configure the recieve buffer
  outl(base_address + 0x44,
       0xf | (1 << 7)); // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP

  uint8_t *send_buffer = ksbrk(0x1000);
  install_handler(test_handler, INT_32_INTERRUPT_GATE(0x0),
                  0x20 + interrupt_line);
  asm("sti");
  kprintf("virt: %x\n", (uint32_t)virtual_to_physical(send_buffer, NULL));
  outl(base_address + 0x20, (uint32_t)virtual_to_physical(send_buffer, NULL));
  for (int i = 0; i < 10; i++) {
    uint8_t status = rtl8139_get_transmit_status(base_address);
    kprintf("status: %x\n", status);
    rtl8139_send_transmit_buffer(base_address, 70);
    status = rtl8139_get_transmit_status(base_address);
    kprintf("status: %x\n", status);

    // Clear TOK(ISR)
    uint32_t tmp = inw(base_address + 0x3E);
    tmp &= ~(1 << 2);
    outw(base_address + 0x3E, tmp);
  }
  for (;;)
    asm("sti");

  //  uint32_t physical_start_address = inl(base_address + 0x20);
}
