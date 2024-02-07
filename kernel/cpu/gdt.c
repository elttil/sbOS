#include "gdt.h"
#include <interrupts.h>

extern void flush_tss(void);
extern void load_gdt(void *);

typedef struct tss_entry_struct tss_entry_t;

tss_entry_t tss_entry;

typedef union {
  struct GDT_Entry s;
  u64 raw;
} GDT_Entry;

GDT_Entry gdt_entries[6] = {0};
GDT_Pointer gdtr;

extern u32 inital_esp;
void write_tss(struct GDT_Entry *gdt_entry) {
  u32 base = (u32)&tss_entry;
  u32 limit = sizeof(tss_entry);

  gdt_entry->limit_low = limit;
  gdt_entry->base_low = base;
  gdt_entry->accessed = 1;
  gdt_entry->read_write = 0;
  gdt_entry->conforming_expand_down = 0;
  gdt_entry->code = 1;
  gdt_entry->code_data_segment = 0;
  gdt_entry->DPL = 0;
  gdt_entry->present = 1;
  gdt_entry->limit_high = limit >> 16;
  gdt_entry->available = 0;
  gdt_entry->long_mode = 0;
  gdt_entry->big = 0;
  gdt_entry->gran = 0;
  gdt_entry->base_high = (base & ((u32)0xff << 24)) >> 24;

  memset(&tss_entry, 0, sizeof tss_entry);
  tss_entry.ss0 = GDT_KERNEL_DATA_SEGMENT * GDT_ENTRY_SIZE;
  register u32 esp asm("esp");
  tss_entry.esp0 = esp;
}

void gdt_init() {
  gdt_entries[GDT_NULL_SEGMENT].raw = 0x0;
  gdt_entries[GDT_KERNEL_CODE_SEGMENT].raw =
      0xCF9A000000FFFF; // Kernel code segment
  gdt_entries[GDT_KERNEL_DATA_SEGMENT].raw =
      0xCF92000000FFFF; // Kernel data segment

  // Usermode code segment
  memcpy(&gdt_entries[GDT_USERMODE_CODE_SEGMENT],
         &gdt_entries[GDT_KERNEL_CODE_SEGMENT], GDT_ENTRY_SIZE);

  // Usermode data segment
  memcpy(&gdt_entries[GDT_USERMODE_DATA_SEGMENT],
         &gdt_entries[GDT_KERNEL_DATA_SEGMENT], GDT_ENTRY_SIZE);

  // Set DPL to 3 to indicate that the segment is in ring 3
  gdt_entries[GDT_USERMODE_CODE_SEGMENT].s.DPL = 3;
  gdt_entries[GDT_USERMODE_DATA_SEGMENT].s.DPL = 3;

  write_tss((struct GDT_Entry *)&gdt_entries[GDT_TSS_SEGMENT]);

  gdtr.offset = (u32)&gdt_entries;
  gdtr.size = sizeof(gdt_entries) - 1;

  disable_interrupts();
  load_gdt(&gdtr);
  flush_tss();
}
