#!/bin/sh
scriptdir="$(dirname "$0")"
cd "$scriptdir"

# Sync the sysroot with the bootable image
mkdir ./mount
sudo mount ext2.img mount
sudo cp -r ../sysroot/* ./mount/
sudo umount mount
rmdir ./mount
