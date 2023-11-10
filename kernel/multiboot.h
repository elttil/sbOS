#include <typedefs.h>
#ifndef MULTIBOOT_HEADER
#define MULTIBOOT_HEADER 1

/* How many bytes from the start of the file we search for the header. */
#define MULTIBOOT_SEARCH 8192
#define MULTIBOOT_HEADER_ALIGN 4

/* The magic field should contain this. */
#define MULTIBOOT_HEADER_MAGIC 0x1BADB002

/* This should be in %eax. */
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

/* Alignment of multiboot modules. */
#define MULTIBOOT_MOD_ALIGN 0x00001000

/* Alignment of the multiboot info structure. */
#define MULTIBOOT_INFO_ALIGN 0x00000004

/* Flags set in the ’flags’ member of the multiboot header. */

/* Align all boot modules on i386 page (4KB) boundaries. */
#define MULTIBOOT_PAGE_ALIGN 0x00000001

/* Must pass memory information to OS. */
#define MULTIBOOT_MEMORY_INFO 0x00000002

/* Must pass video information to OS. */
#define MULTIBOOT_VIDEO_MODE 0x00000004

/* This flag indicates the use of the address fields in the header. */
#define MULTIBOOT_AOUT_KLUDGE 0x00010000

/* Flags to be set in the ’flags’ member of the multiboot info structure. */

/* is there basic lower/upper memory information? */
#define MULTIBOOT_INFO_MEMORY 0x00000001
/* is there a boot device set? */
#define MULTIBOOT_INFO_BOOTDEV 0x00000002
/* is the command-line defined? */
#define MULTIBOOT_INFO_CMDLINE 0x00000004
/* are there modules to do something with? */
#define MULTIBOOT_INFO_MODS 0x00000008

/* These next two are mutually exclusive */

/* is there a symbol table loaded? */
#define MULTIBOOT_INFO_AOUT_SYMS 0x00000010
/* is there an ELF section header table? */
#define MULTIBOOT_INFO_ELF_SHDR 0X00000020

/* is there a full memory map? */
#define MULTIBOOT_INFO_MEM_MAP 0x00000040

/* Is there drive info? */
#define MULTIBOOT_INFO_DRIVE_INFO 0x00000080

/* Is there a config table? */
#define MULTIBOOT_INFO_CONFIG_TABLE 0x00000100

/* Is there a boot loader name? */
#define MULTIBOOT_INFO_BOOT_LOADER_NAME 0x00000200

/* Is there a APM table? */
#define MULTIBOOT_INFO_APM_TABLE 0x00000400

/* Is there video information? */
#define MULTIBOOT_INFO_VBE_INFO 0x00000800
#define MULTIBOOT_INFO_FRAMEBUFFER_INFO 0x00001000

#ifndef ASM_FILE

typedef u8 multiboot_u8;
typedef u16 multiboot_u16;
typedef u32 multiboot_u32;
typedef u64 multiboot_u64;

struct multiboot_header {
  /* Must be MULTIBOOT_MAGIC - see above. */
  multiboot_u32 magic;

  /* Feature flags. */
  multiboot_u32 flags;

  /* The above fields plus this one must equal 0 mod 2^32. */
  multiboot_u32 checksum;

  /* These are only valid if MULTIBOOT_AOUT_KLUDGE is set. */
  multiboot_u32 header_addr;
  multiboot_u32 load_addr;
  multiboot_u32 load_end_addr;
  multiboot_u32 bss_end_addr;
  multiboot_u32 entry_addr;

  /* These are only valid if MULTIBOOT_VIDEO_MODE is set. */
  multiboot_u32 mode_type;
  multiboot_u32 width;
  multiboot_u32 height;
  multiboot_u32 depth;
};

/* The symbol table for a.out. */
struct multiboot_aout_symbol_table {
  multiboot_u32 tabsize;
  multiboot_u32 strsize;
  multiboot_u32 addr;
  multiboot_u32 reserved;
};
typedef struct multiboot_aout_symbol_table multiboot_aout_symbol_table_t;

/* The section header table for ELF. */
struct multiboot_elf_section_header_table {
  multiboot_u32 num;
  multiboot_u32 size;
  multiboot_u32 addr;
  multiboot_u32 shndx;
};
typedef struct multiboot_elf_section_header_table
    multiboot_elf_section_header_table_t;

struct multiboot_info {
  /* Multiboot info version number */
  multiboot_u32 flags;

  /* Available memory from BIOS */
  multiboot_u32 mem_lower;
  multiboot_u32 mem_upper;

  /* "root" partition */
  multiboot_u32 boot_device;

  /* Kernel command line */
  multiboot_u32 cmdline;

  /* Boot-Module list */
  multiboot_u32 mods_count;
  multiboot_u32 mods_addr;

  union {
    multiboot_aout_symbol_table_t aout_sym;
    multiboot_elf_section_header_table_t elf_sec;
  } u;

  /* Memory Mapping buffer */
  multiboot_u32 mmap_length;
  multiboot_u32 mmap_addr;

  /* Drive Info buffer */
  multiboot_u32 drives_length;
  multiboot_u32 drives_addr;

  /* ROM configuration table */
  multiboot_u32 config_table;

  /* Boot Loader Name */
  multiboot_u32 boot_loader_name;

  /* APM table */
  multiboot_u32 apm_table;

  /* Video */
  multiboot_u32 vbe_control_info;
  multiboot_u32 vbe_mode_info;
  multiboot_u16 vbe_mode;
  multiboot_u16 vbe_interface_seg;
  multiboot_u16 vbe_interface_off;
  multiboot_u16 vbe_interface_len;

  multiboot_u64 framebuffer_addr;
  multiboot_u32 framebuffer_pitch;
  multiboot_u32 framebuffer_width;
  multiboot_u32 framebuffer_height;
  multiboot_u8 framebuffer_bpp;
#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED 0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB 1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT 2
  multiboot_u8 framebuffer_type;
  union {
    struct {
      multiboot_u32 framebuffer_palette_addr;
      multiboot_u16 framebuffer_palette_num_colors;
    };
    struct {
      multiboot_u8 framebuffer_red_field_position;
      multiboot_u8 framebuffer_red_mask_size;
      multiboot_u8 framebuffer_green_field_position;
      multiboot_u8 framebuffer_green_mask_size;
      multiboot_u8 framebuffer_blue_field_position;
      multiboot_u8 framebuffer_blue_mask_size;
    };
  };
};
typedef struct multiboot_info multiboot_info_t;

struct multiboot_color {
  multiboot_u8 red;
  multiboot_u8 green;
  multiboot_u8 blue;
};

struct multiboot_mmap_entry {
  multiboot_u32 size;
  multiboot_u64 addr;
  multiboot_u64 len;
#define MULTIBOOT_MEMORY_AVAILABLE 1
#define MULTIBOOT_MEMORY_RESERVED 2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS 4
#define MULTIBOOT_MEMORY_BADRAM 5
  multiboot_u32 type;
} __attribute__((packed));
typedef struct multiboot_mmap_entry multiboot_memory_map_t;

struct multiboot_mod_list {
  /* the memory used goes from bytes ’mod_start’ to ’mod_end-1’ inclusive */
  multiboot_u32 mod_start;
  multiboot_u32 mod_end;

  /* Module command line */
  multiboot_u32 cmdline;

  /* padding to take it to 16 bytes (must be zero) */
  multiboot_u32 pad;
};
typedef struct multiboot_mod_list multiboot_module_t;

/* APM BIOS info. */
struct multiboot_apm_info {
  multiboot_u16 version;
  multiboot_u16 cseg;
  multiboot_u32 offset;
  multiboot_u16 cseg_16;
  multiboot_u16 dseg;
  multiboot_u16 flags;
  multiboot_u16 cseg_len;
  multiboot_u16 cseg_16_len;
  multiboot_u16 dseg_len;
};

#endif /* ! ASM_FILE */

#endif /* ! MULTIBOOT_HEADER */
