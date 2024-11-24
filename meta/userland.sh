#!/bin/sh
scriptdir="$(dirname "$0")"
cd "$scriptdir"
export PATH="$PATH:$(pwd)/../toolchain/bin/bin"
cd ..
make -j`nproc` -C ./userland/libgui
make -j`nproc` -C ./userland/libc
mkdir -p ./sysroot/lib
make install -C ./userland/libc
make -j`nproc` -C ./userland/terminal
make -j`nproc` -C ./userland/snake
make -j`nproc` -C ./userland/ante
make -j`nproc` -C ./userland/windowserver
make -j`nproc` -C ./userland/test
make -j`nproc` -C ./userland/minibox
make -j`nproc` -C ./userland/libppm
make -j`nproc` -C ./userland/rtl8139
make -j`nproc` -C ./userland/smol_http
make -j`nproc` -C ./userland/irc
make -j`nproc` -C ./userland/nasm-2.16.01
make -j`nproc` -C ./userland/dns
make -j`nproc` -C ./userland/to
make -j`nproc` -C ./userland/httpd

mkdir sysroot
sudo cp ./userland/rtl8139/rtl8139 ./sysroot/rtl8139
sudo cp ./userland/libppm/ppm ./sysroot/ppm
sudo cp ./userland/terminal/term ./sysroot/term
sudo cp ./userland/snake/snake ./sysroot/snake
sudo cp ./userland/ante/ante ./sysroot/ante
sudo cp ./userland/windowserver/ws ./sysroot/ws
sudo cp ./userland/test/test ./sysroot/test
sudo cp ./userland/minibox/minibox ./sysroot/minibox
sudo cp ./userland/smol_http/smol_http ./sysroot/smol_http
sudo cp ./userland/irc/irc ./sysroot/irc
sudo cp ./userland/nasm-2.16.01/nasm ./sysroot/nasm
sudo cp ./userland/dns/dns ./sysroot/dns
sudo cp ./userland/to/to ./sysroot/to
sudo cp ./userland/httpd/httpd ./sysroot/httpd

cd ./sysroot
rm ./init
rm ./cat
rm ./yes
rm ./echo
rm ./wc
rm ./ls
ln -s ./minibox ./init
ln -s ./minibox ./cat
ln -s ./minibox ./yes
ln -s ./minibox ./echo
ln -s ./minibox ./wc
ln -s ./minibox ./ls
ln -s ./minibox ./kill
ln -s ./minibox ./sha1sum
ln -s ./minibox ./rdate
ln -s ./minibox ./sh
cd ..
