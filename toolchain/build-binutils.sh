#!/bin/sh
./download-binutils.sh
tar -xf binutils*.tar.xz
cd ./binutils-*/
patch -f -p1 -i ../binutils-2.40.diff
cd ..
mkdir bin
PREFIX="$(pwd)/bin"
mkdir build-binutils
cd build-binutils
../binutils*/configure --target=i686-sb --prefix="$PREFIX" --with-sysroot="$(pwd)/../../sysroot" --disable-werror
make -j$(nproc) && make install
