#ifndef GDT_H
#define GDT_H
#include <typedefs.h>
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
	u16 limit_low;
	u32 base_low               : 24;
	u32 accessed               :  1;
	u32 read_write             :  1; // readable for code, writable for data
	u32 conforming_expand_down :  1; // conforming for code, expand down for data
	u32 code                   :  1; // 1 for code, 0 for data
	u32 code_data_segment      :  1; // should be 1 for everything but TSS and LDT
	u32 DPL                    :  2; // privilege level
	u32 present                :  1;
	u32 limit_high             :  4;
	u32 available              :  1; // only used in software; has no effect on hardware
	u32 long_mode              :  1;
	u32 big                    :  1; // 32-bit opcodes for code, u32 stack for data
	u32 gran                   :  1; // 1 to use 4k page addressing, 0 for byte addressing
	u8 base_high;
}__attribute__((packed));

typedef struct GDT_Pointer
{
	u16 size;
	u32 offset;
}__attribute__((packed)) GDT_Pointer;

struct tss_entry_struct
{
	u32 prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
	u32 esp0;     // The stack pointer to load when changing to kernel mode.
	u32 ss0;      // The stack segment to load when changing to kernel mode.
	// Everything below here is unused.
	u32 esp1; // esp and ss 1 and 2 would be used when switching to rings 1 or 2.
	u32 ss1;
	u32 esp2;
	u32 ss2;
	u32 cr3;
	u32 eip;
	u32 eflags;
	u32 eax;
	u32 ecx;
	u32 edx;
	u32 ebx;
	u32 esp;
	u32 ebp;
	u32 esi;
	u32 edi;
	u32 es;
	u32 cs;
	u32 ss;
	u32 ds;
	u32 fs;
	u32 gs;
	u32 ldt;
	u16 trap;
	u16 iomap_base;
} __attribute__((packed));

void gdt_init();

u8 gen_access_byte(u8 priv, u8 s, u8 ex, u8 dc, u8 rw);
u64 gen_gdt_entry(u32 base, u32 limit, u8 access_byte, u8 flag);
u8 gen_flag(u8 gr, u8 sz);
#endif
