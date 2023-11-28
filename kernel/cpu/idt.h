typedef struct kernel_registers kernel_registers_t;
typedef struct registers registers_t;
#ifndef IDT_H
#define IDT_H
#include <cpu/gdt.h>
#include <cpu/io.h>
#include <log.h>
#include <stdio.h>
#include <typedefs.h>

/*
 * the type_attribute in the IDT_Entry struct
 * is divded like this
 *   7                           0
 * +---+---+---+---+---+---+---+---+
 * | P |  DPL  | S |    GateType   |
 * +---+---+---+---+---+---+---+---+
 * It is 8 bits(1 byte) long
 *
 * P
 *	Present bit. Should be zero for unused
 *	interrupts.
 *
 * DPL
 *	Specifices the maximum ring(0 to 3) the
 *	interrupt can be called from.
 *
 * S
 *	Storage segment. This should be set to
 *	zero for all interrupt and trap gates.
 *
 * GateType
 *	Possible IDT gate types:
 *	0b0101	0x5	5	80386 32 bit task gate
 *	0b0110	0x6	6	80286 16-bit interrupt gate
 *	0b0111	0x7	7	80286 16-bit trap gate
 *	0b1110	0xE	14	80386 32-bit interrupt gate
 *	0b1111	0xF	15	80386 32-bit trap gate
 */

// This enables the present bit.
#define INT_PRESENT 0x80 /* 0b10000000 */

#define INT_32_TASK_GATE(min_privlege)                                         \
  (INT_PRESENT | 0x05 | (min_privlege << 5))
#define INT_16_INTERRUPT_GATE(min_privlege)                                    \
  (INT_PRESENT | 0x06 | (min_privlege << 5))
#define INT_16_TRAP_GATE(min_privlege)                                         \
  (INT_PRESENT | 0x07 | (min_privlege << 5))
#define INT_32_INTERRUPT_GATE(min_privlege)                                    \
  (INT_PRESENT | 0x0E | (min_privlege << 5))
#define INT_32_TRAP_GATE(min_privlege)                                         \
  (INT_PRESENT | 0x0F | (min_privlege << 5))

struct interrupt_frame;

struct registers {
  u32 error_code;
  u32 eip;
  u32 cs;
  u32 eflags;
  u32 esp;
  u32 ss;
};

void idt_init(void);
__attribute__((no_caller_saved_registers)) void EOI(unsigned char irq);
void install_handler(void (*handler_function)(), u16 type_attribute, u8 entry);
#endif
