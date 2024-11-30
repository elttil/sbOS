#include <assert.h>
#include <audio.h>
#include <cpu/arch_inst.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <cpu/spinlock.h>
#include <cpu/syscall.h>
#include <crypto/SHA1/sha1.h>
#include <drivers/ahci.h>
#include <drivers/ata.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/pit.h>
#include <drivers/rtl8139.h>
#include <drivers/serial.h>
#include <drivers/vbe.h>
#include <fcntl.h>
#include <fs/devfs.h>
#include <fs/ext2.h>
#include <fs/shm.h>
#include <fs/vfs.h>
#include <interrupts.h>
#include <log.h>
#include <mmu.h>
#include <multiboot.h>
#include <network/arp.h>
#include <random.h>
#include <sched/scheduler.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <timer.h>
#include <typedefs.h>

#if defined(__linux__)
#error "You are not using a cross-compiler."
#endif

#if !defined(__i386__)
#error "This OS needs to be compiled with a ix86-elf compiler"
#endif

u32 inital_esp;
uintptr_t data_end;

void kernel_main(u32 kernel_end, unsigned long magic, unsigned long addr,
                 u32 inital_stack) {
  data_end = 0xc0400000;
  inital_esp = inital_stack;

  disable_interrupts();
  kprintf("If you see this then the serial driver works :D.\n");
  assert(magic == MULTIBOOT_BOOTLOADER_MAGIC);

  multiboot_info_t *mb = (multiboot_info_t *)(addr + 0xc0000000);
  u32 mem_kb = mb->mem_lower;
  u32 mem_mb = (mb->mem_upper - 1000) / 1000;
  u64 memsize_kb = mem_mb * 1000 + mem_kb;

  paging_init(memsize_kb, mb);
  klog(LOG_SUCCESS, "Paging Initalized");
  mb = mmu_map_frames((multiboot_info_t *)addr, sizeof(multiboot_info_t));
  assert(mb);
  assert(display_driver_init(mb));

  gdt_init();
  klog(LOG_SUCCESS, "GDT Initalized");

  idt_init();
  klog(LOG_SUCCESS, "IDT Initalized");

  setup_random();

  timer_start_init();

  syscalls_init();
  klog(LOG_SUCCESS, "Syscalls Initalized");

  pit_install();
  set_pit_count(2);
  klog(LOG_SUCCESS, "PIT driver installed");

  ata_init();
  klog(LOG_SUCCESS, "ATA Initalized");

  tasking_init();
  klog(LOG_SUCCESS, "Tasking Initalized");

  install_mouse();
  klog(LOG_SUCCESS, "PS2 Mouse driver installed");

  install_keyboard();
  klog(LOG_SUCCESS, "PS2 Keyboard driver installed");

  global_socket_init();

  vfs_mount("/dev", devfs_mount());
  assert(ahci_init());
  vfs_inode_t *ext2_mount_point = ext2_mount();
  assert(ext2_mount_point);
  vfs_mount("/", ext2_mount_point);

  add_stdout();
  add_serial();
  add_random_devices();

  timer_wait_for_init();
  timer_add_clock();

  shm_init();

  add_keyboard();
  add_mouse();

  rtl8139_init();

  ipv4_t gateway;
  gen_ipv4(&gateway, 10, 0, 2, 2);
  ipv4_t bitmask;
  gen_ipv4(&bitmask, 255, 255, 255, 0);
  setup_network(gateway, bitmask);

  gen_ipv4(&ip_address, 10, 0, 2, 15);
  enable_interrupts();

  audio_init();

  add_vbe_device();

  int pid;
  if (0 == (pid = fork())) {
    char *argv[] = {"/init", NULL};
    if (0 == exec("/init", argv, 0, 0)) {
      kprintf("exec() failed\n");
    }
  }
  for (;;) {
    current_task->sleep_until = timer_get_uptime() + 100000000;
    wait_for_interrupt();
  }
}
