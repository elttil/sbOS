#ifndef GDT_H
#define GDT_H
#include <stdint.h>
#include <string.h>

#define GDT_ENTRY_SIZE 0x8
#define GDT_NULL_SEGMENT 0x0
#define GDT_KERNEL_CODE_SEGMENT 0x1
#define GDT_KERNEL_DATA_SEGMENT 0x2
#define GDT_USERMODE_CODE_SEGMENT 0x3
#define GDT_USERMODE_DATA_SEGMENT 0x4
#define GDT_TSS_SEGMENT 0x5

struct GDT_Entry
{
	uint16_t limit_low;
	uint32_t base_low               : 24;
	uint32_t accessed               :  1;
	uint32_t read_write             :  1; // readable for code, writable for data
	uint32_t conforming_expand_down :  1; // conforming for code, expand down for data
	uint32_t code                   :  1; // 1 for code, 0 for data
	uint32_t code_data_segment      :  1; // should be 1 for everything but TSS and LDT
	uint32_t DPL                    :  2; // privilege level
	uint32_t present                :  1;
	uint32_t limit_high             :  4;
	uint32_t available              :  1; // only used in software; has no effect on hardware
	uint32_t long_mode              :  1;
	uint32_t big                    :  1; // 32-bit opcodes for code, uint32_t stack for data
	uint32_t gran                   :  1; // 1 to use 4k page addressing, 0 for byte addressing
	uint8_t base_high;
}__attribute__((packed));

typedef struct GDT_Pointer
{
	uint16_t size;
	uint32_t offset;
}__attribute__((packed)) GDT_Pointer;

struct tss_entry_struct
{
	uint32_t prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
	uint32_t esp0;     // The stack pointer to load when changing to kernel mode.
	uint32_t ss0;      // The stack segment to load when changing to kernel mode.
	// Everything below here is unused.
	uint32_t esp1; // esp and ss 1 and 2 would be used when switching to rings 1 or 2.
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} __attribute__((packed));

void gdt_init();

uint8_t gen_access_byte(uint8_t priv, uint8_t s, uint8_t ex, uint8_t dc, uint8_t rw);
uint64_t gen_gdt_entry(uint32_t base, uint32_t limit, uint8_t access_byte, uint8_t flag);
uint8_t gen_flag(uint8_t gr, uint8_t sz);
#endif
