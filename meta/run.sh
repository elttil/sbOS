#!/bin/sh
scriptdir="$(dirname "$0")"
cd "$scriptdir"
cd ..
#qemu-system-i386 -netdev user,id=n0,hostfwd=tcp:127.0.0.1:6001-:6000 -device rtl8139,netdev=n0 -object filter-dump,id=id,netdev=n0,file=./logs/netout -d int -no-reboot -no-shutdown -chardev stdio,id=char0,logfile=./logs/serial.log,signal=off -serial chardev:char0 -drive id=disk,file=./meta/ext2.img,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -m 512M -cdrom ./kernel/myos.iso -s
qemu-system-i386 -netdev user,id=n0,hostfwd=tcp:127.0.0.1:6001-:6000 -device rtl8139,netdev=n0 -object filter-dump,id=id,netdev=n0,file=./logs/netout -no-reboot -no-shutdown -chardev stdio,id=char0,logfile=./logs/serial.log,signal=off -serial chardev:char0 -drive id=disk,file=./meta/ext2.img,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -m 512M -cdrom ./kernel/myos.iso -s
# Sync the sysroot
cd ./meta/
mkdir ./mount
sudo mount ./ext2.img ./mount
sudo cp -r ./mount/* ../sysroot/
sudo umount ./ext2.img
