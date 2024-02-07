#include <assert.h>
#include <cpu/idt.h>
#include <cpu/io.h>
#include <drivers/pci.h>
#include <drivers/rtl8139.h>
#include <ipc.h>
#include <mmu.h>
#include <network/arp.h>
#include <network/ethernet.h>

#define RBSTART 0x30
#define CMD 0x37
#define IMR 0x3C

#define RTL8139_RXBUFFER_SIZE (8192 + 16)

#define TSD0 0x10  // transmit status
#define TSAD0 0x20 // transmit start address

struct PCI_DEVICE rtl8139;
u8 *device_buffer;

u8 *send_buffers[4];
int send_buffers_loop = 0;

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

u32 current_packet_read = 0;

void handle_packet(void) {
  assert(sizeof(struct _INT_PACKET_HEADER) == sizeof(u16));

  for (int i = 0; 0 == (inb(rtl8139.gen.base_mem_io + 0x37) & 1); i++) {
    u16 *buf = (u16 *)(device_buffer + current_packet_read);
    struct PACKET_HEADER packet_header;
    packet_header.raw = *buf;
    if (packet_header.data.FAE) {
      return;
    }
    if (packet_header.data.CRC) {
      return;
    }
    if (!packet_header.data.ROK) {
      return;
    }
    u16 packet_length = *(buf + 1);
    assert(packet_length <= 2048);

    u8 packet_buffer[RTL8139_RXBUFFER_SIZE];
    if (current_packet_read + packet_length >= RTL8139_RXBUFFER_SIZE) {
      u32 end = RTL8139_RXBUFFER_SIZE - current_packet_read;
      memcpy(packet_buffer, buf, end);
      u32 rest = packet_length - end;
      memcpy(packet_buffer + end, device_buffer, rest);
    } else {
      memcpy(packet_buffer, buf, packet_length);
    }

    // I have no documentation backing this implementation of updating
    // the CBR. It is just a (somewhat)uneducated guess. But it does
    // seem to work.
    u32 old = current_packet_read;
    current_packet_read = (current_packet_read + packet_length + 4 + 3) & (~3);
    current_packet_read %= RTL8139_RXBUFFER_SIZE;
    outw(rtl8139.gen.base_mem_io + 0x38, current_packet_read - 0x10);
    current_packet_read = inw(rtl8139.gen.base_mem_io + 0x3A);
    outw(rtl8139.gen.base_mem_io + 0x38, current_packet_read - 0x10);

    assert(current_packet_read != old);

    handle_ethernet((u8 *)packet_buffer + 4, packet_length);
  }
}

__attribute__((interrupt)) void rtl8139_handler(void *regs) {
  (void)regs;
  u16 status = inw(rtl8139.gen.base_mem_io + 0x3e);

  if (status & (1 << 2)) {
  }
  if (status & (1 << 0)) {
    handle_packet();
  }
  outw(rtl8139.gen.base_mem_io + 0x3E, 0x5);

  EOI(0xB);
}

void rtl8139_send_data(u8 *data, u16 data_size) {
  kprintf("ipc_write\n");
  ipc_write(0, data, data_size);
  /*
  const struct PCI_DEVICE *device = &rtl8139;
  // FIXME: It should block or fail if there is too little space for the
  // buffer
  if (data_size > 0x1000) {
    rtl8139_send_data(data, 0x1000);
    data += 0x1000;
    data_size -= 0x1000;
    return rtl8139_send_data(data, data_size);
  }
  if (send_buffers_loop > 3) {
    send_buffers_loop = 0;
  }
  memcpy(send_buffers[send_buffers_loop], data, data_size);
  outl(device->gen.base_mem_io + 0x20 + send_buffers_loop * 4,
       (u32)virtual_to_physical(send_buffers[send_buffers_loop], NULL));
  outl(device->gen.base_mem_io + 0x10 + send_buffers_loop * 4, data_size);
  send_buffers_loop += 1;*/
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
  device_buffer = ksbrk(RTL8139_RXBUFFER_SIZE);
  memset(device_buffer, 0, RTL8139_RXBUFFER_SIZE);
  // Setupt the recieve buffer
  u32 rx_buffer = (u32)virtual_to_physical(device_buffer, NULL);
  outl(base_address + RBSTART, rx_buffer);

  // Set IMR + ISR
  //  outw(base_address + IMR, (1 << 2) | (1 << 3) | (1 << 0));
  outw(base_address + IMR, (1 << 2) | (1 << 0));

  // Set transmit and reciever enable
  outb(base_address + 0x37, (1 << 2) | (1 << 3));

  // Configure the recieve buffer
  outl(base_address + 0x44,
       0xf); // 0xf is AB+AM+APM+AAP

  install_handler(rtl8139_handler, INT_32_INTERRUPT_GATE(0x3),
                  0x20 + interrupt_line);

  // ksbrk() seems to have the magical ability of disabling interrupts?
  // I have no fucking clue why that happens and it was a pain to debug.
  for (int i = 0; i < 4; i++)
    send_buffers[i] = ksbrk(0x1000);
}
