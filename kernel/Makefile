CC="i686-sb-gcc"
AS="i686-sb-as"
OBJ = arch/i386/boot.o init/kernel.o cpu/gdt.o cpu/reload_gdt.o cpu/idt.o cpu/io.o libc/stdio/print.o drivers/keyboard.o log.o drivers/pit.o libc/string/memcpy.o libc/string/strlen.o libc/string/memcmp.o drivers/ata.o libc/string/memset.o cpu/syscall.o  read_eip.o libc/exit/assert.o process.o libc/string/strcpy.o arch/i386/mmu.o kmalloc.o fs/ext2.o fs/vfs.o fs/devfs.o cpu/spinlock.o random.o libc/string/strcmp.o crypto/ChaCha20/chacha20.o crypto/SHA1/sha1.o fs/tmpfs.o libc/string/isequal.o drivers/pst.o syscalls/ppoll.o syscalls/ftruncate.o kubsan.o syscalls/mmap.o drivers/serial.o syscalls/accept.o syscalls/bind.o syscalls/socket.o socket.o poll.o fs/fifo.o hashmap/hashmap.o fs/shm.o syscalls/shm.o elf.o ksbrk.o sched/scheduler.o syscalls/fstat.o libc/string/copy.o drivers/mouse.o libc/string/strlcpy.o libc/string/strcat.o drivers/vbe.o syscalls/msleep.o syscalls/uptime.o syscalls/mkdir.o drivers/pci.o drivers/rtl8139.o network/ethernet.o network/arp.o network/bytes.o network/ipv4.o network/udp.o syscalls/recvfrom.o math.o syscalls/sendto.o signal.o syscalls/kill.o syscalls/sigaction.o network/tcp.o drivers/ahci.o crypto/xoshiro256plusplus/xoshiro256plusplus.o syscalls/chdir.o syscalls/getcwd.o syscalls/isatty.o syscalls/randomfill.o syscalls/open.o syscalls/write.o syscalls/pwrite.o syscalls/port.o syscalls/map_frames.o syscalls/virtual_to_physical.o syscalls/install_irq.o arch/i386/interrupts.o cpu/isr.o lib/stack.o lib/buffered_write.o lib/list.o cpu/arch_inst.o cpu/int_syscall.o syscalls/munmap.o syscalls/open_process.o syscalls/lseek.o lib/ringbuffer.o lib/relist.o
CFLAGS = -std=c99 -O0 -fsanitize=vla-bound,shift-exponent,pointer-overflow,shift,signed-integer-overflow,bounds -ggdb -ffreestanding -Wall -Wextra -Wno-int-conversion -Wno-unused-parameter -Werror -mgeneral-regs-only -Wimplicit-fallthrough -I./libc/include/ -I. -Wno-pointer-sign -DKERNEL
LDFLAGS=
INCLUDE=-I./includes/ -I../include/ -I./libc/include/

all: myos.iso

%.o: %.c
	clang-format -i $<
	$(CC) $(INCLUDE) -c -o $@ $< $(CFLAGS)

%.o: %.s
	$(AS) $< -o $@

myos.bin: $(OBJ)
	$(CC) $(INCLUDE) $(LDFLAGS) -shared -T linker.ld -o myos.bin -ffreestanding -nostdlib $(CFLAGS) $^ -lgcc

myos.iso: myos.bin
	cp myos.bin isodir/boot
	grub-mkrescue -o myos.iso isodir

clean:
	rm myos.bin myos.iso $(OBJ)
