#!/bin/sh
qemu-system-i386 -no-reboot -no-shutdown -serial file:./logs/serial.log -hda ext2.img -m 1G -cdrom ./kernel/myos.iso -s -S &
gdb -x .gdbinit
