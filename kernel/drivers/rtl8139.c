#include <assert.h>
#include <cpu/idt.h>
#include <cpu/io.h>
#include <drivers/pci.h>
#include <drivers/rtl8139.h>
#include <interrupts.h>
#include <mmu.h>
#include <network/arp.h>
#include <network/ethernet.h>

#define RBSTART 0x30
#define CMD 0x37
#define IMR 0x3C

// 0 - 8K
// 1 - 16K
// 2 - 32K
// 3 - 64K
unsigned int rtl8139_chosen_buffer = 3;
#define RTL8139_RXBUFFER_SIZE (u32)(8192 << rtl8139_chosen_buffer)

#define TSD0 0x10  // transmit status
#define TSAD0 0x20 // transmit start address

struct PCI_DEVICE rtl8139;
u8 *device_buffer;

u8 *send_buffers[4];
int send_buffers_loop = 0;

struct IntPacketHeader {
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
} __attribute__((packed));

struct RTL8139PacketHeader {
  union {
    u16 raw;
    struct IntPacketHeader data;
  };
} __attribute__((packed));

unsigned short current_packet_read = 0;

void handle_packet(void) {
  assert(sizeof(struct IntPacketHeader) == sizeof(u16));

  int had_error = 0;

  for (; 0 == (inb(rtl8139.gen.base_mem_io + 0x37) & 1);) {
    u32 ring_offset = current_packet_read % RTL8139_RXBUFFER_SIZE;

    u16 rx_size = *(u16 *)(device_buffer + ring_offset + sizeof(u16));

    struct RTL8139PacketHeader packet_header;
    packet_header.raw = device_buffer[ring_offset + 0];

    int error = (packet_header.data.FAE) || (packet_header.data.CRC) ||
                (!packet_header.data.ROK);

    if (error) {
      current_packet_read = 0;
      outb(rtl8139.gen.base_mem_io + 0x37, 0x4);
      outb(rtl8139.gen.base_mem_io + 0x37, 0x4 | 0x8);
      had_error = 1;
    } else {
      int packet_length = rx_size - 4;
      assert(packet_length <= 2048);

      u8 packet_buffer[packet_length];
      if (ring_offset + rx_size > RTL8139_RXBUFFER_SIZE) {
        int end = RTL8139_RXBUFFER_SIZE - ring_offset - 4;
        memcpy(packet_buffer, &device_buffer[ring_offset + 4], end);
        memcpy(packet_buffer + end, device_buffer, packet_length - end);
      } else {
        memcpy(packet_buffer, &device_buffer[ring_offset + 4], packet_length);
      }

      handle_ethernet(packet_buffer, packet_length);
    }

    current_packet_read = (current_packet_read + rx_size + 4 + 3) & (~3);
    outw(rtl8139.gen.base_mem_io + 0x38, current_packet_read - 0x10);
  }
  if (had_error) {
    current_packet_read = 0;
  }
}

void rtl8139_handler(void *regs) {
  (void)regs;
  u16 status = inw(rtl8139.gen.base_mem_io + 0x3e);

  outw(rtl8139.gen.base_mem_io + 0x3E, 0x5);
  if (status & (1 << 2)) {
  }
  if (status & (1 << 0)) {
    handle_packet();
  }

  EOI(0xB);
}

void rtl8139_send_data(u8 *data, u16 data_size) {
  if (data_size > 0x1000) {
    rtl8139_send_data(data, 0x1000);
    data += 0x1000;
    data_size -= 0x1000;
    return rtl8139_send_data(data, data_size);
  }
  const struct PCI_DEVICE *device = &rtl8139;
  if (send_buffers_loop > 3) {
    send_buffers_loop = 0;
  }
  memcpy(send_buffers[send_buffers_loop], data, data_size);
  outl(device->gen.base_mem_io + 0x20 + send_buffers_loop * 4,
       (u32)virtual_to_physical(send_buffers[send_buffers_loop], NULL));
  outl(device->gen.base_mem_io + 0x10 + send_buffers_loop * 4, data_size);
  send_buffers_loop += 1;
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
  if ((status_register >> 3) & 0x1) {
    kprintf("transmit error :(\n");
  }
  u8 status = (status_register >> 2) & 0x1;
  outl(base_address + 0x3E, 0x5);
  return status;
}

void rtl8139_init(void) {
  if (!pci_populate_device_struct(0x10EC, 0x8139, &rtl8139)) {
    return;
  }
  klog(LOG_NOTE, "RTL8139 found at bus: %x slot: %x\n", rtl8139.bus,
       rtl8139.slot);

  for (int i = 0; i < 4; i++) {
    send_buffers[i] = kmalloc_align(0x1000, NULL);
    if (!send_buffers[i]) {
      goto rtl8139_error_memory;
    }
  }

  // Try to chose the largest buffer possible without running out of
  // memory.
  // NOTE: This is so rare it should probably not even be here. I just
  // thought it was a cool idea.
  u32 rx_buffer;
  for (;;) {
    // NOTE: RTL8139_RXBUFFER_SIZE is a macro that contains
    // "rtl8139_chosen_buffer"
    device_buffer =
        kmalloc_align(RTL8139_RXBUFFER_SIZE + 16, (void **)&rx_buffer);
    if (!device_buffer) {
      if (0 == rtl8139_chosen_buffer) {
        goto rtl8139_error_memory;
      }
      rtl8139_chosen_buffer--;
      continue;
    }
    break;
  }

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
  // Setupt the recieve buffer
  outl(base_address + RBSTART, rx_buffer);

  // Set IMR + ISR
  outw(base_address + IMR, (1 << 0));

  // Set transmit and reciever enable
  outb(base_address + 0x37, (1 << 2) | (1 << 3));

  // TODO: Test these later as I believe they can have performance
  // implications but currently the OS seems to be bottlenecked by other
  // things.
  u8 fifo_threshold = 0;
  u8 dma_burst = 0;

  u8 packet_accept = 0xe; // 0xe is AB+AM+APM

  // Configure the recieve buffer
  outl(base_address + 0x44, packet_accept | (rtl8139_chosen_buffer << 11) |
                                (fifo_threshold << 13) | (dma_burst << 8));

  install_handler((interrupt_handler)rtl8139_handler,
                  INT_32_INTERRUPT_GATE(0x3), 0x20 + interrupt_line);
  return;
rtl8139_error_memory:
  klog(LOG_ERROR, "Failed to initalize RTL8139 driver: Out of memory");
  for (int i = 0; i < 4; i++) {
    kmalloc_align_free(send_buffers[i], 0x1000);
  }
}
