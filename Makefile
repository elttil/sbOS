CC="./sysroot/bin/i686-sb-gcc"
AS="./sysroot/bin/i686-sb-as"
OBJ = arch/i386/boot.o init/kernel.o cpu/gdt.o cpu/reload_gdt.o cpu/idt.o cpu/io.o libc/stdio/print.o drivers/keyboard.o log.o drivers/pit.o libc/string/memcpy.o libc/string/strlen.o libc/string/memcmp.o drivers/ata.o libc/string/memset.o cpu/syscall.o  read_eip.o libc/exit/assert.o process.o cpu/int_syscall.o libc/string/strcpy.o arch/i386/mmu.o kmalloc.o fs/ext2.o fs/vfs.o fs/devfs.o cpu/spinlock.o random.o libc/string/strcmp.o crypto/ChaCha20/chacha20.o crypto/SHA1/sha1.o fs/tmpfs.o libc/string/isequal.o drivers/pst.o halts.o scalls/ppoll.o scalls/ftruncate.o kubsan.o scalls/mmap.o drivers/serial.o scalls/accept.o scalls/bind.o scalls/socket.o socket.o poll.o fs/fifo.o hashmap/hashmap.o fs/shm.o scalls/shm.o elf.o ksbrk.o sched/scheduler.o scalls/stat.o libc/string/copy.o libc/string/strncpy.o drivers/mouse.o libc/string/strlcpy.o libc/string/strcat.o drivers/vbe.o scalls/msleep.o scalls/uptime.o scalls/mkdir.o drivers/pci.o drivers/rtl8139.o
CFLAGS = -O2 -fsanitize=vla-bound,shift-exponent,pointer-overflow,shift,signed-integer-overflow,bounds -ggdb -ffreestanding -Wall -Werror -mgeneral-regs-only -Wimplicit-fallthrough -I./libc/include/ -I.
INCLUDE=-I./includes/ -I./libc/include/

all: myos.iso

%.o: %.c
	$(CC) $(INCLUDE) -c -o $@ $< $(CFLAGS)

%.o: %.s
	$(AS) $< -o $@

myos.bin: $(OBJ)
	$(CC) $(INCLUDE) -shared -T linker.ld -o myos.bin -ffreestanding -nostdlib $(CFLAGS) $^ -lgcc

debug:
	qemu-system-i386 -no-reboot -no-shutdown -serial file:./serial.log -hda ext2.img -m 1G -cdrom myos.iso -s -S &
	gdb -x .gdbinit

nk:
	qemu-system-i386 -netdev tap,ifname=tap0,id=network0,script=no,downscript=no -device rtl8139,netdev=network0 -d int -no-reboot -no-shutdown -serial file:./serial.log -hda ext2.img -m 1G -cdrom myos.iso -s

run:
	qemu-system-i386 -enable-kvm -netdev tap,ifname=tap0,id=network0,script=no,downscript=no -device rtl8139,netdev=network0 -d int -no-reboot -no-shutdown -chardev stdio,id=char0,logfile=serial.log,signal=off -serial chardev:char0 -hda ext2.img -m 1G -cdrom myos.iso -s

myos.iso: myos.bin
	cp myos.bin isodir/boot
	grub-mkrescue -o myos.iso isodir

clean:
	rm myos.bin myos.iso $(OBJ)
