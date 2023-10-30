#!/bin/sh
# If the filesystem becomes corrupted this shell script is used to
# completly reset it
scriptdir="$(dirname "$0")"
cd "$scriptdir"
# Include /sbin to the PATH since mkfs.ext2 exists only in sbin on debian :/
export PATH="$PATH:/sbin"
rm ext2.img
mkfs.ext2 ext2.img 20M
./sync.sh
