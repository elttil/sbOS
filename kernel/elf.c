#include <assert.h>
#include <crypto/SHA1/sha1.h>
#include <elf.h>
#include <fcntl.h>
#include <sched/scheduler.h>
#include <stddef.h>
#include <typedefs.h>

void *load_elf_file(const char *f, u32 *ds) {
  ELFHeader header;
  int fd = vfs_open(f, O_RDONLY, 0);
  if (fd < 0) {
    return NULL;
  }

  if (sizeof(header) != vfs_pread(fd, &header, sizeof(header), 0)) {
    return NULL;
  }

  if (0 != memcmp(header.e_ident, "\x7F\x45\x4C\x46" /* "\x7FELF" */, 4)) {
    return NULL;
  }

  Elf32_Phdr program_header;
  assert(sizeof(program_header) == header.e_phentsize);
  u32 header_offset = header.e_phoff;
  uintptr_t end_of_code = 0;
  for (int i = 0; i < header.e_phnum;
       i++, header_offset += header.e_phentsize) {
    if (0 >=
        vfs_pread(fd, &program_header, sizeof(program_header), header_offset)) {
      return NULL;
    }

    // FIXME: Only one type is supported, which is 1(load). More should be
    // added.
    assert(1 == program_header.p_type);

    // 1. Clear p_memsz bytes at p_vaddr to 0.(We also allocate frames for
    // that range)
    u32 p_memsz = program_header.p_memsz;
    u32 p_vaddr = program_header.p_vaddr;

    u32 pages_to_allocate = (u32)align_page((void *)(p_vaddr + p_memsz));
    pages_to_allocate -= p_vaddr - (p_vaddr % 0x1000);
    pages_to_allocate /= 0x1000;

    if (!mmu_allocate_region((void *)p_vaddr, pages_to_allocate * 0x1000,
                             MMU_FLAG_RW, NULL)) {
      return NULL;
    }

    uintptr_t e = program_header.p_vaddr + program_header.p_memsz;
    if (e > end_of_code) {
      end_of_code = e;
    }

    memset((void *)program_header.p_vaddr, 0, program_header.p_memsz);

    // 2. Copy p_filesz bytes from p_offset to p_vaddr
    int rc = vfs_pread(fd, (void *)program_header.p_vaddr,
                       program_header.p_filesz, program_header.p_offset);

    assert(rc == (int)program_header.p_filesz);
  }
  *ds = end_of_code;
  vfs_close(fd);
  return (void *)header.e_entry;
}
