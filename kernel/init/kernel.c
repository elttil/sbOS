#include <assert.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <cpu/spinlock.h>
#include <cpu/syscall.h>
#include <crypto/SHA1/sha1.h>
#include <drivers/ata.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/pit.h>
#include <drivers/rtl8139.h>
#include <drivers/serial.h>
#include <drivers/vbe.h>
#include <fs/devfs.h>
#include <fs/ext2.h>
#include <fs/shm.h>
#include <fs/vfs.h>
#include <log.h>
#include <mmu.h>
#include <multiboot.h>
#include <random.h>
#include <sched/scheduler.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(__linux__)
#error "You are not using a cross-compiler."
#endif

#if !defined(__i386__)
#error "This OS needs to be compiled with a ix86-elf compiler"
#endif

uint32_t inital_esp;
uintptr_t data_end;

void kernel_main(uint32_t kernel_end, unsigned long magic, unsigned long addr,
                 uint32_t inital_stack) {
  (void)kernel_end;
  data_end = 0xc0400000;
  inital_esp = inital_stack;

  asm("cli");
  kprintf("If you see this then the serial driver works :D.\n");
  assert(magic == MULTIBOOT_BOOTLOADER_MAGIC);

  multiboot_info_t *mb = (multiboot_info_t *)(addr + 0xc0000000);
  uint32_t mem_kb = mb->mem_lower;
  uint32_t mem_mb = (mb->mem_upper - 1000) / 1000;
  uint64_t memsize_kb = mem_mb * 1000 + mem_kb;
  paging_init(memsize_kb);
  klog("Paging Initalized", LOG_SUCCESS);
  mb = mmu_map_frames((multiboot_info_t *)addr, sizeof(multiboot_info_t));

  gdt_init();
  klog("GDT Initalized", LOG_SUCCESS);

  idt_init();
  klog("IDT Initalized", LOG_SUCCESS);

  syscalls_init();
  klog("Syscalls Initalized", LOG_SUCCESS);

  pit_install();
  set_pit_count(2000);
  klog("PIT driver installed", LOG_SUCCESS);

  ata_init();
  klog("ATA Initalized", LOG_SUCCESS);

  tasking_init();
  klog("Tasking Initalized", LOG_SUCCESS);

  install_mouse();
  klog("PS2 Mouse driver installed", LOG_SUCCESS);

  install_keyboard();
  klog("PS2 Keyboard driver installed", LOG_SUCCESS);

  vfs_mount("/", ext2_mount());
  vfs_mount("/dev", devfs_mount());
  add_stdout();
  add_serial();
  add_random_devices();
  shm_init();

  setup_random();

  add_keyboard();
  add_mouse();
  rtl8139_init();

  display_driver_init(mb);
  add_vbe_device();
  int pid;
  if (0 == (pid = fork())) {
    char *argv[] = {"/init", NULL};
    if (0 == exec("/init", argv)) {
      kprintf("exec() failed\n");
    }
  }
  for (;;)
    ;
}
