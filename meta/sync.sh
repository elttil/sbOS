#!/bin/sh
scriptdir="$(dirname "$0")"
cd "$scriptdir"

[ -f ./ext2.img ] || (./new.sh ; exit)

# Sync the sysroot with the bootable image
mkdir ./mount
sudo mount ext2.img mount
sudo rsync ../sysroot/ ./mount/
sudo umount mount
rmdir ./mount
