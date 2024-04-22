#ifndef ELF_H
#define ELF_H
#include <assert.h>
#ifdef KERNEL
#include <fs/vfs.h>
#include <mmu.h>
#endif
#include <typedefs.h>

#define EI_NIDENT 16

#define SHN_UNDEF 0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2
#define SHN_HIRESERVE 0xffff

#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF32_R_INFO(s, t) (((s) << 8) + (unsigned char)(t))

#define ELF32_ST_BIND(i) ((i)>>4)
#define ELF32_ST_TYPE(i) ((i)&0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

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

#define Elf32_Addr u32  // Unsigned program address
#define Elf32_Half u16  // Unsigned medium integer
#define Elf32_Off u32   // Unsigned file offset
#define Elf32_Sword u32 // Signed large integer
#define Elf32_Word u32  // Unsigned large integer

#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_LOPROC 13
#define STT_HIPROC 15

#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_MASKPROC 0xf0000000

#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_LOPROC 13
#define STB_HIPROC 15

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define R_386_NONE 0
#define R_386_32 1
#define R_386_PC32 2
#define R_386_GOT32 3
#define R_386_PLT32 4
#define R_386_COPY 5
#define R_386_GLOB_DAT 6
#define R_386_JMP_SLOT 7
#define R_386_RELATIVE 8
#define R_386_GOTOFF 9
#define R_386_GOTPC 10

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

typedef struct {
  Elf32_Addr r_offset;
  Elf32_Word r_info;
} Elf32_Rel;

typedef struct {
  Elf32_Word st_name;
  Elf32_Addr st_value;
  Elf32_Word st_size;
  unsigned char st_info;
  unsigned char st_other;
  Elf32_Half st_shndx;
} Elf32_Sym;

typedef struct {
  unsigned char e_ident[EI_NIDENT];
  Elf32_Half e_type;
  Elf32_Half e_machine;
  Elf32_Word e_version;
  Elf32_Addr e_entry;
  Elf32_Off e_phoff;
  Elf32_Off e_shoff;
  Elf32_Word e_flags;
  Elf32_Half e_ehsize;
  Elf32_Half e_phentsize;
  Elf32_Half e_phnum;
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx;
} Elf32_Ehdr;

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

void *load_elf_file(const char *f, u32 *ds);
#endif
