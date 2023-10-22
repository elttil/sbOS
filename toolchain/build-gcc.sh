#!/bin/sh
./download-gcc.sh
tar -xf gcc-*.tar.xz
cd ./gcc-*/
patch -f -p1 -i ../gcc-13.1.0.diff
cd ..
mkdir bin
PREFIX=$(pwd)"/bin"
#PREFIX="/home/anton/prj/osdev/sysroot"
mkdir build-gcc
cd build-gcc
../gcc-*/configure --target=i686-sb --prefix="$PREFIX" --with-gmp --with-mpfr --with-sysroot="/home/anton/prj/osdev/sysroot" --enable-languages=c,c++
make -j6 all-gcc all-target-libgcc && make install-gcc install-target-libgcc
