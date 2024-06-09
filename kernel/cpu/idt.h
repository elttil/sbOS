#ifndef IDT_H
#define IDT_H
#include <cpu/gdt.h>
#include <cpu/io.h>
#include <log.h>
#include <stdio.h>
#include <typedefs.h>
typedef struct kernel_registers kernel_registers_t;
typedef struct registers registers_t;

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

void isr0();
void isr1();
void isr2();
void isr3();
void isr4();
void isr5();
void isr6();
void isr7();
void isr8();
void isr9();
void isr10();
void isr11();
void isr12();
void isr13();
void isr14();
void isr15();
void isr16();
void isr17();
void isr18();
void isr19();
void isr20();
void isr21();
void isr22();
void isr23();
void isr24();
void isr25();
void isr26();
void isr27();
void isr28();
void isr29();
void isr30();
void isr31();
void isr32();
void isr33();
void isr34();
void isr35();
void isr36();
void isr37();
void isr38();
void isr39();
void isr40();
void isr41();
void isr42();
void isr43();
void isr44();
void isr45();
void isr46();
void isr47();
void isr48();
void isr49();
void isr50();
void isr51();
void isr52();
void isr53();
void isr54();
void isr55();
void isr56();
void isr57();
void isr58();
void isr59();
void isr60();
void isr61();
void isr62();
void isr63();
void isr64();
void isr65();
void isr66();
void isr67();
void isr68();
void isr69();
void isr70();
void isr71();
void isr72();
void isr73();
void isr74();
void isr75();
void isr76();
void isr77();
void isr78();
void isr79();
void isr80();
void isr81();
void isr82();
void isr83();
void isr84();
void isr85();
void isr86();
void isr87();
void isr88();
void isr89();
void isr90();
void isr91();
void isr92();
void isr93();
void isr94();
void isr95();
void isr96();
void isr97();
void isr98();
void isr99();
void isr100();
void isr101();
void isr102();
void isr103();
void isr104();
void isr105();
void isr106();
void isr107();
void isr108();
void isr109();
void isr110();
void isr111();
void isr112();
void isr113();
void isr114();
void isr115();
void isr116();
void isr117();
void isr118();
void isr119();
void isr120();
void isr121();
void isr122();
void isr123();
void isr124();
void isr125();
void isr126();
void isr127();
void isr128();
void isr129();
void isr130();
void isr131();
void isr132();
void isr133();
void isr134();
void isr135();
void isr136();
void isr137();
void isr138();
void isr139();
void isr140();
void isr141();
void isr142();
void isr143();
void isr144();
void isr145();
void isr146();
void isr147();
void isr148();
void isr149();
void isr150();
void isr151();
void isr152();
void isr153();
void isr154();
void isr155();
void isr156();
void isr157();
void isr158();
void isr159();
void isr160();
void isr161();
void isr162();
void isr163();
void isr164();
void isr165();
void isr166();
void isr167();
void isr168();
void isr169();
void isr170();
void isr171();
void isr172();
void isr173();
void isr174();
void isr175();
void isr176();
void isr177();
void isr178();
void isr179();
void isr180();
void isr181();
void isr182();
void isr183();
void isr184();
void isr185();
void isr186();
void isr187();
void isr188();
void isr189();
void isr190();
void isr191();
void isr192();
void isr193();
void isr194();
void isr195();
void isr196();
void isr197();
void isr198();
void isr199();
void isr200();
void isr201();
void isr202();
void isr203();
void isr204();
void isr205();
void isr206();
void isr207();
void isr208();
void isr209();
void isr210();
void isr211();
void isr212();
void isr213();
void isr214();
void isr215();
void isr216();
void isr217();
void isr218();
void isr219();
void isr220();
void isr221();
void isr222();
void isr223();
void isr224();
void isr225();
void isr226();
void isr227();
void isr228();
void isr229();
void isr230();
void isr231();
void isr232();
void isr233();
void isr234();
void isr235();
void isr236();
void isr237();
void isr238();
void isr239();
void isr240();
void isr241();
void isr242();
void isr243();
void isr244();
void isr245();
void isr246();
void isr247();
void isr248();
void isr249();
void isr250();
void isr251();
void isr252();
void isr253();
void isr254();
void isr255();

typedef struct reg {
  u32 ds;
  // Pushed by pusha.
  u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
  u32 int_no, error_code;
  // Pushed by the processor automatically.
  u32 eip, cs, eflags, useresp, ss;
} reg_t;

void idt_init(void);
__attribute__((no_caller_saved_registers)) void EOI(unsigned char irq);

typedef void (*interrupt_handler)(reg_t *);
void install_handler(interrupt_handler handler_function, u16 type_attribute,
                     u8 entry);
void irq_set_mask(u8 irq_line);
void irq_clear_mask(u8 irq_line);
#endif
