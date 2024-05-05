#include <cpu/arch_inst.h>
#include <cpu/idt.h>
#include <interrupts.h>
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

__attribute__((no_caller_saved_registers)) void EOI(u8 irq) {
  if (irq > 7) {
    outb(SLAVE_PIC_COMMAND_PORT, 0x20);
  }

  outb(MASTER_PIC_COMMAND_PORT, 0x20);
}

void general_protection_fault(reg_t *regs) {
  klog("General Protetion Fault", 0x1);
  kprintf(" Error Code: %x\n", regs->error_code);
  kprintf("Instruction Pointer: %x\n", regs->eip);
  if (current_task) {
    kprintf("PID: %x\n", current_task->pid);
    kprintf("Name: %s\n", current_task->program_name);
  }
  dump_backtrace(12);
  halt();
  EOI(0xD - 8);
}

void double_fault(registers_t *regs) {
  (void)regs;
  klog("DOUBLE FAULT", LOG_ERROR);
  halt();
}

void invalid_opcode(reg_t *regs) {
  klog("Invalid opcode", LOG_ERROR);
  kprintf("Instruction Pointer: %x\n", regs->eip);
  dump_backtrace(8);
  halt();
}

void page_fault(reg_t *regs) {
  uint32_t cr2 = get_cr2();
  if (0xDEADC0DE == cr2) {
    disable_interrupts();
    EOI(0xB);
    process_pop_restore_context(NULL, regs);
    return;
  }

  int is_userspace = (regs->error_code & (1 << 2));
  if (is_userspace) {
    signal_process(current_task, SIGSEGV);
    return;
  }

  klog("Page Fault", LOG_ERROR);
  kprintf("CR2: %x\n", cr2);
  if (current_task) {
    kprintf("PID: %x\n", current_task->pid);
    kprintf("Name: %s\n", current_task->program_name);
  }
  kprintf("Error Code: %x\n", regs->error_code);
  kprintf("Instruction Pointer: %x\n", regs->eip);

  if (regs->error_code & (1 << 0)) {
    kprintf("page-protection violation\n");
  } else {
    kprintf("non-present page\n");
  }

  if (regs->error_code & (1 << 1)) {
    kprintf("write access\n");
  } else {
    kprintf("read access\n");
  }

  if (regs->error_code & (1 << 4)) {
    kprintf("Attempted instruction fetch\n");
  }

  dump_backtrace(15);
  halt();
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

void pic_remap(int offset) {
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

void irq_set_mask(u8 irq_line) {
  u16 port;
  u8 value;
  port = (irq_line < 8) ? MASTER_PIC_DATA_PORT : SLAVE_PIC_DATA_PORT;
  if (irq_line >= 8) {
    irq_line -= 8;
  }
  value = inb(port) | (1 << irq_line);
  outb(port, value);
}

void irq_clear_mask(u8 irq_line) {
  u16 port;
  u8 value;
  port = (irq_line < 8) ? MASTER_PIC_DATA_PORT : SLAVE_PIC_DATA_PORT;
  if (irq_line >= 8) {
    irq_line -= 8;
  }
  value = inb(port) & ~(1 << irq_line);
  outb(port, value);
}

// TODO: Maybe lay these out using assembly, macro or via a linker
// script?
void *isr_list[] = {
    isr0,   isr1,   isr2,   isr3,   isr4,   isr5,   isr6,   isr7,   isr8,
    isr9,   isr10,  isr11,  isr12,  isr13,  isr14,  isr15,  isr16,  isr17,
    isr18,  isr19,  isr20,  isr21,  isr22,  isr23,  isr24,  isr25,  isr26,
    isr27,  isr28,  isr29,  isr30,  isr31,  isr32,  isr33,  isr34,  isr35,
    isr36,  isr37,  isr38,  isr39,  isr40,  isr41,  isr42,  isr43,  isr44,
    isr45,  isr46,  isr47,  isr48,  isr49,  isr50,  isr51,  isr52,  isr53,
    isr54,  isr55,  isr56,  isr57,  isr58,  isr59,  isr60,  isr61,  isr62,
    isr63,  isr64,  isr65,  isr66,  isr67,  isr68,  isr69,  isr70,  isr71,
    isr72,  isr73,  isr74,  isr75,  isr76,  isr77,  isr78,  isr79,  isr80,
    isr81,  isr82,  isr83,  isr84,  isr85,  isr86,  isr87,  isr88,  isr89,
    isr90,  isr91,  isr92,  isr93,  isr94,  isr95,  isr96,  isr97,  isr98,
    isr99,  isr100, isr101, isr102, isr103, isr104, isr105, isr106, isr107,
    isr108, isr109, isr110, isr111, isr112, isr113, isr114, isr115, isr116,
    isr117, isr118, isr119, isr120, isr121, isr122, isr123, isr124, isr125,
    isr126, isr127, isr128, isr129, isr130, isr131, isr132, isr133, isr134,
    isr135, isr136, isr137, isr138, isr139, isr140, isr141, isr142, isr143,
    isr144, isr145, isr146, isr147, isr148, isr149, isr150, isr151, isr152,
    isr153, isr154, isr155, isr156, isr157, isr158, isr159, isr160, isr161,
    isr162, isr163, isr164, isr165, isr166, isr167, isr168, isr169, isr170,
    isr171, isr172, isr173, isr174, isr175, isr176, isr177, isr178, isr179,
    isr180, isr181, isr182, isr183, isr184, isr185, isr186, isr187, isr188,
    isr189, isr190, isr191, isr192, isr193, isr194, isr195, isr196, isr197,
    isr198, isr199, isr200, isr201, isr202, isr203, isr204, isr205, isr206,
    isr207, isr208, isr209, isr210, isr211, isr212, isr213, isr214, isr215,
    isr216, isr217, isr218, isr219, isr220, isr221, isr222, isr223, isr224,
    isr225, isr226, isr227, isr228, isr229, isr230, isr231, isr232, isr233,
    isr234, isr235, isr236, isr237, isr238, isr239, isr240, isr241, isr242,
    isr243, isr244, isr245, isr246, isr247, isr248, isr249, isr250, isr251,
    isr252, isr253, isr254, isr255,
};

interrupt_handler list_of_handlers[256];

void int_handler(reg_t *r) {
  const interrupt_handler handler = list_of_handlers[r->int_no];
  if (NULL == handler) {
    kprintf("[NOTE] Interrupt(0x%x) called but has no interrupt handler",
            r->int_no);
    return;
  }

  handler(r);

  int is_kernel = (r->cs == 0x08);
  if (!is_kernel) {
    const signal_t *sig = process_pop_signal(NULL);
    if (sig) {
      process_push_restore_context(NULL, *r);
      r->eip = sig->handler_ip;

      // Add magic value to the stack such that the signal handler
      // returns to 0xDEADC0DE
      r->useresp -= 4;
      *(u32 *)r->useresp = 0xDEADC0DE;
    }
  }
}

void install_handler(interrupt_handler handler_function, u16 type_attribute,
                     u8 entry) {
  format_descriptor((u32)isr_list[entry], KERNEL_CODE_SEGMENT_OFFSET,
                    type_attribute, &IDT_Entry[entry]);
  list_of_handlers[entry] = (interrupt_handler)handler_function;
  if (entry >= 0x20 && entry <= 0x30) {
    irq_clear_mask(entry - 0x20);
  }
}

void idt_init(void) {
  memset(list_of_handlers, 0, sizeof(void *) * 256);

  pic_remap(0x20);
  for (int i = 0; i < 0x10; i++) {
    irq_set_mask(i);
  }

  install_handler((interrupt_handler)invalid_opcode, INT_32_INTERRUPT_GATE(0x0),
                  0x6);
  install_handler((interrupt_handler)page_fault, INT_32_INTERRUPT_GATE(0x0),
                  0xE);
  install_handler((interrupt_handler)double_fault, INT_32_INTERRUPT_GATE(0x0),
                  0x8);
  install_handler((interrupt_handler)general_protection_fault,
                  INT_32_INTERRUPT_GATE(0x0), 0xD);

  idtr.interrupt_table = (struct IDT_Descriptor **)&IDT_Entry;
  idtr.size = (sizeof(struct IDT_Descriptor) * IDT_MAX_ENTRY) - 1;
  load_idtr(&idtr);
}
