CC="i686-sb-gcc"
AS="i686-sb-as"
OBJ = arch/i386/boot.o init/kernel.o cpu/gdt.o cpu/reload_gdt.o cpu/idt.o cpu/io.o libc/stdio/print.o drivers/keyboard.o log.o drivers/pit.o libc/string/memcpy.o libc/string/strlen.o libc/string/memcmp.o drivers/ata.o libc/string/memset.o cpu/syscall.o  read_eip.o libc/exit/assert.o process.o libc/string/strcpy.o arch/i386/mmu.o kmalloc.o fs/ext2.o fs/vfs.o fs/devfs.o cpu/spinlock.o random.o libc/string/strcmp.o crypto/ChaCha20/chacha20.o crypto/SHA1/sha1.o fs/tmpfs.o libc/string/isequal.o drivers/pst.o kubsan.o drivers/serial.o socket.o poll.o fs/fifo.o hashmap/hashmap.o fs/shm.o elf.o ksbrk.o sched/scheduler.o libc/string/copy.o drivers/mouse.o libc/string/strlcpy.o libc/string/strcat.o drivers/vbe.o drivers/pci.o drivers/rtl8139.o network/ethernet.o network/arp.o network/bytes.o network/ipv4.o network/udp.o math.o signal.o network/tcp.o drivers/ahci.o crypto/xoshiro256plusplus/xoshiro256plusplus.o arch/i386/interrupts.o cpu/isr.o lib/stack.o lib/buffered_write.o lib/list.o cpu/arch_inst.o cpu/int_syscall.o lib/ringbuffer.o lib/relist.o arch/i386/tsc.o arch/i386/asm_tsc.o drivers/cmos.o timer.o queue.o fonts.o drivers/ac97.o audio.o
#CFLAGS = -std=c99 -O3 -fsanitize=vla-bound,shift-exponent,pointer-overflow,shift,signed-integer-overflow,bounds -ggdb -ffreestanding -Wall -Wextra -Wno-int-conversion -Wno-unused-parameter -Werror -mgeneral-regs-only -Wimplicit-fallthrough -I./libc/include/ -I. -Wno-pointer-sign -DKERNEL
#LDFLAGS=
CFLAGS = -std=c99 -O3 -flto -ggdb -ffreestanding -Wall -Wextra -Wno-int-conversion -Wno-unused-parameter -Werror -mgeneral-regs-only -Wimplicit-fallthrough -I./libc/include/ -I. -Wno-pointer-sign -DKERNEL
LDFLAGS= -O3 -flto
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
