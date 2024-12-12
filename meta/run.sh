#!/bin/sh
scriptdir="$(dirname "$0")"
cd "$scriptdir"
cd ..
#qemu-system-i386 -enable-kvm \
#~/rg/qemu-9.0.1/build/qemu-system-i386 -display sdl -enable-kvm \
#	-device virtio-net-pci,netdev=n0,mac=00:00:11:11:11:11 \
#~/rg/qemu/build/qemu-system-i386 -d int -display sdl \
~/rg/qemu/build/qemu-system-i386 -enable-kvm -display sdl \
	-netdev user,id=n0,hostfwd=tcp:127.0.0.1:6001-:80,hostfwd=tcp:127.0.0.1:6000-:22\
	-object filter-dump,maxlen=1024000,id=id,netdev=n0,file=./logs/netout\
	-no-reboot -no-shutdown\
	-device rtl8139,netdev=n0\
	-chardev stdio,id=char0,logfile=./logs/serial.log,signal=off\
	-serial chardev:char0 -drive id=disk,file=./meta/ext2.img,if=none\
	-device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0\
	-audio driver=sdl,model=ac97,id=0\
	-m 2G -cdrom ./kernel/myos.iso -s # 2> interrupts
# Sync the sysroot
cd ./meta/
mkdir ./mount
sudo mount ./ext2.img ./mount
sudo rsync ./mount/ ../sysroot/
sudo umount ./ext2.img
