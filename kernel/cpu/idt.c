#include <cpu/idt.h>
#include <sched/scheduler.h>
#include <stdio.h>

#define MASTER_PIC_COMMAND_PORT 0x20
#define MASTER_PIC_DATA_PORT 0x21
#define SLAVE_PIC_COMMAND_PORT 0xA0
#define SLAVE_PIC_DATA_PORT 0xA1
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KERNEL_CODE_SEGMENT_OFFSET GDT_ENTRY_SIZE *GDT_KERNEL_CODE_SEGMENT

#define IDT_MAX_ENTRY 256

struct IDT_Descriptor {
  u16 low_offset;
  u16 code_segment_selector;
  u8 zero; // Always should be zero
  u8 type_attribute;
  u16 high_offset;
} __attribute__((packed)) __attribute__((aligned(4)));

struct IDT_Pointer {
  u16 size;
  struct IDT_Descriptor **interrupt_table;
} __attribute__((packed));

struct IDT_Descriptor IDT_Entry[IDT_MAX_ENTRY];
struct IDT_Pointer idtr;

extern void load_idtr(void *idtr);

void format_descriptor(u32 offset, u16 code_segment, u8 type_attribute,
                       struct IDT_Descriptor *descriptor) {
  descriptor->low_offset = offset & 0xFFFF;
  descriptor->high_offset = offset >> 16;
  descriptor->type_attribute = type_attribute;
  descriptor->code_segment_selector = code_segment;
  descriptor->zero = 0;
}

void install_handler(void (*handler_function)(), u16 type_attribute, u8 entry) {
  format_descriptor((u32)handler_function, KERNEL_CODE_SEGMENT_OFFSET,
                    type_attribute, &IDT_Entry[entry]);
}

__attribute__((no_caller_saved_registers)) void EOI(u8 irq) {
  if (irq > 7)
    outb(SLAVE_PIC_COMMAND_PORT, 0x20);

  outb(MASTER_PIC_COMMAND_PORT, 0x20);
}

__attribute__((interrupt)) void general_protection_fault(registers_t *regs) {
  klog("General Protetion Fault", 0x1);
  kprintf(" Error Code: %x\n", regs->error_code);
  kprintf("Instruction Pointer: %x\n", regs->eip);
  dump_backtrace(12);
  asm("hlt");
  for (;;)
    ;
  EOI(0xD - 8);
}

__attribute__((interrupt)) void double_fault(registers_t *regs) {
  (void)regs;
  klog("DOUBLE FAULT", LOG_ERROR);
  asm("hlt");
  for (;;)
    ;
}
__attribute__((interrupt)) void page_fault(registers_t *regs) {
  if (0xFFFFDEAD == regs->eip) {
    asm("sti");
    for (;;)
      switch_task();
  }
  klog("Page Fault", LOG_ERROR);
  if (get_current_task()) {
    kprintf("PID: %x\n", get_current_task()->pid);
    kprintf("Name: %s\n", get_current_task()->program_name);
  }
  kprintf("Error Code: %x\n", regs->error_code);
  kprintf("Instruction Pointer: %x\n", regs->eip);

  volatile uint32_t cr2;
  asm volatile("mov %%cr2, %0" : "=r"(cr2));
  kprintf("CR2: %x\n", cr2);

  if (regs->error_code & (1 << 0))
    kprintf("page-protection violation\n");
  else
    kprintf("non-present page\n");

  if (regs->error_code & (1 << 1))
    kprintf("write access\n");
  else
    kprintf("read access\n");

  if (regs->error_code & (1 << 2))
    kprintf("CPL = 3\n");

  if (regs->error_code & (1 << 4))
    kprintf("Attempted instruction fetch\n");

  dump_backtrace(5);
  asm("hlt");
  for (;;)
    ;
}

static inline void io_wait(void) {
  outb(0x80, 0);
}

#define ICW1_ICW4 0x01      /* ICW4 (not) needed */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10      /* Initialization - required! */

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

void PIC_remap(int offset) {
  unsigned char a1, a2;
  a1 = inb(MASTER_PIC_DATA_PORT);
  a2 = inb(SLAVE_PIC_DATA_PORT);

  // Send ICW1 and tell the PIC that we will issue a ICW4
  // Since there is no ICW1_SINGLE sent to indicate it is cascaded
  // it means we will issue a ICW3.
  outb(MASTER_PIC_COMMAND_PORT, ICW1_INIT | ICW1_ICW4);
  io_wait();
  outb(SLAVE_PIC_COMMAND_PORT, ICW1_INIT | ICW1_ICW4);
  io_wait();

  // As a part of ICW2 this sends the offsets(the position to put IRQ0) of the
  // vector tables.
  outb(MASTER_PIC_DATA_PORT, offset);
  io_wait();
  outb(SLAVE_PIC_DATA_PORT, offset + 0x8);
  io_wait();

  // This tells the master on which lines it is having slaves
  outb(MASTER_PIC_DATA_PORT, 4);
  io_wait();
  // This tells the slave the cascading identity.
  outb(SLAVE_PIC_DATA_PORT, 2);
  io_wait();

  outb(MASTER_PIC_DATA_PORT, ICW4_8086);
  io_wait();
  outb(SLAVE_PIC_DATA_PORT, ICW4_8086);
  io_wait();

  outb(MASTER_PIC_DATA_PORT, a1);
  outb(SLAVE_PIC_DATA_PORT, a2);
}

void IRQ_set_mask(unsigned char IRQline) {
  u16 port;
  u8 value;
  port = (IRQline < 8) ? MASTER_PIC_DATA_PORT : SLAVE_PIC_DATA_PORT;
  if (IRQline >= 8)
    IRQline -= 8;
  value = inb(port) | (1 << IRQline);
  outb(port, value);
}

void IRQ_clear_mask(unsigned char IRQline) {
  u16 port;
  u8 value;
  port = (IRQline < 8) ? MASTER_PIC_DATA_PORT : SLAVE_PIC_DATA_PORT;
  if (IRQline >= 8) {
    IRQline -= 8;
  }
  value = inb(port) & ~(1 << IRQline);
  outb(port, value);
}

void idt_init(void) {
  install_handler(page_fault, INT_32_INTERRUPT_GATE(0x0), 0xE);
  install_handler(double_fault, INT_32_INTERRUPT_GATE(0x0), 0x8);
  install_handler(general_protection_fault, INT_32_INTERRUPT_GATE(0x0), 0xD);

  PIC_remap(0x20);
  IRQ_clear_mask(0xb);
  IRQ_set_mask(0xe);
  IRQ_set_mask(0xf);
  IRQ_clear_mask(2);

  idtr.interrupt_table = (struct IDT_Descriptor **)&IDT_Entry;
  idtr.size = (sizeof(struct IDT_Descriptor) * IDT_MAX_ENTRY) - 1;
  load_idtr(&idtr);
}
