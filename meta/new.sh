#!/bin/sh
# If the filesystem becomes corrupted this shell script is used to
# completly reset it
scriptdir="$(dirname "$0")"
cd "$scriptdir"
rm ext2.img
mkfs.ext2 ext2.img 20M
./sync.sh
