#ifndef ELF_H
#define ELF_H
#include <stdint.h>
#include <mmu.h>
#include <fs/vfs.h>
#include <assert.h>

#define ET_NONE 0        // No file type
#define ET_REL 1         // Relocatable file
#define ET_EXEC 2        // Executable file
#define ET_DYN 3         // Shared object file
#define ET_CORE 4        // Core file
#define ET_LOPROC 0xff00 // Processor-specific
#define ET_HIPROC 0xffff // Processor-specific

#define EM_NONE 0  // No machine
#define EM_M32 1   // AT&T WE 32100
#define EM_SPARC 2 // SPARC
#define EM_386 3   // Intel 80386
#define EM_68K 4   // Motorola 68000
#define EM_88K 5   // Motorola 88000
#define EM_860 7   // Intel 80860
#define EM_MIPS 8  // MIPS RS3000

#define EV_NONE 0    // Invalid version
#define EV_CURRENT 1 // Current version

#define ELF_EXECUTABLE (1 << 0)
#define ELF_WRITABLE (1 << 1)
#define ELF_READABLE (1 << 2)

#define Elf32_Addr uint32_t  // Unsigned program address
#define Elf32_Half uint16_t  // Unsigned medium integer
#define Elf32_Off uint32_t   // Unsigned file offset
#define Elf32_Sword uint32_t // Signed large integer
#define Elf32_Word uint32_t  // Unsigned large integer

#define ELF_EXEC (1 << 0)
#define ELF_WRITE (1 << 1)
#define ELF_READ (1 << 2)

// ELF header
typedef struct {
  unsigned char e_ident[16];
  Elf32_Half e_type;    // Object file type      (ET_*)
  Elf32_Half e_machine; // Required architecture (EM_*)
  Elf32_Word e_version; // Object file version   (EV_*)
  Elf32_Addr e_entry;   // File entry point
  Elf32_Off e_phoff;    // Program header table's offset(bytes)
  Elf32_Off e_shoff;    // Section header table's offset(bytes)
  Elf32_Word e_flags;
  Elf32_Half e_ehsize; // ELF Header size
  Elf32_Half
      e_phentsize;    // Size of program's header tables(all are the same size)
  Elf32_Half e_phnum; // Amount of program headers
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx;
} __attribute__((packed)) ELFHeader;

// Section header
typedef struct {
  Elf32_Word sh_name;
  Elf32_Word sh_type;
  Elf32_Word sh_flags;
  Elf32_Addr sh_addr;
  Elf32_Off sh_offset;
  Elf32_Word sh_size;
  Elf32_Word sh_link;
  Elf32_Word sh_info;
  Elf32_Word sh_addralign;
  Elf32_Word sh_entsize;
} Elf32_Shdr;

enum ShT_Types {
  SHT_NULL = 0,     // Null section
  SHT_PROGBITS = 1, // Program information
  SHT_SYMTAB = 2,   // Symbol table
  SHT_STRTAB = 3,   // String table
  SHT_RELA = 4,     // Relocation (w/ addend)
  SHT_NOBITS = 8,   // Not present in file
  SHT_REL = 9,      // Relocation (no addend)
};

// Program header
typedef struct {
  Elf32_Word p_type;
  Elf32_Off p_offset;
  Elf32_Addr p_vaddr;
  Elf32_Addr p_paddr;
  Elf32_Word p_filesz;
  Elf32_Word p_memsz;
  Elf32_Word p_flags;
  Elf32_Word p_align;
} __attribute__((packed)) Elf32_Phdr;


void *load_elf_file(const char *f, uint32_t *ds);
#endif
