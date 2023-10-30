#!/bin/sh
# This script compiles the operating system toolchain(GCC, Binutils)
scriptdir="$(dirname "$0")"
cd "$scriptdir"
cd ..

# Install the headers from LibC
mkdir -p ./sysroot/usr/include/
cp -r ./userland/libc/include ./sysroot/usr/

cd ./toolchain/
./build-binutils.sh
./build-gcc.sh
