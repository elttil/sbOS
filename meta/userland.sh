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
make -j`nproc` -C ./userland/tcpserver
make -j`nproc` -C ./userland/sftp
make -j`nproc` -C ./userland/tunneld

mkdir sysroot
mkdir ./sysroot/bin
sudo cp ./userland/libppm/ppm ./sysroot/bin/ppm
sudo cp ./userland/terminal/term ./sysroot/bin/term
sudo cp ./userland/snake/snake ./sysroot/bin/snake
sudo cp ./userland/ante/ante ./sysroot/bin/ante
sudo cp ./userland/windowserver/ws ./sysroot/bin/ws
sudo cp ./userland/test/test ./sysroot/bin/test
sudo cp ./userland/minibox/minibox ./sysroot/bin/minibox
sudo cp ./userland/smol_http/smol_http ./sysroot/bin/smol_http
sudo cp ./userland/irc/irc ./sysroot/bin/irc
sudo cp ./userland/nasm-2.16.01/nasm ./sysroot/bin/nasm
sudo cp ./userland/dns/dns ./sysroot/bin/dns
sudo cp ./userland/to/to ./sysroot/bin/to
sudo cp ./userland/httpd/httpd ./sysroot/bin/httpd
sudo cp ./userland/tcpserver/tcpserver ./sysroot/bin/tcpserver
sudo cp ./userland/sftp/sftp ./sysroot/bin/sftp
sudo cp ./userland/tunneld/tunneld ./sysroot/bin/tunneld

cd ./sysroot
rm ./bin/init
rm ./bin/cat
rm ./bin/yes
rm ./bin/echo
rm ./bin/wc
rm ./bin/ls
ln -s ./bin/minibox ./bin/init
ln -s ./bin/minibox ./bin/cat
ln -s ./bin/minibox ./bin/yes
ln -s ./bin/minibox ./bin/echo
ln -s ./bin/minibox ./bin/wc
ln -s ./bin/minibox ./bin/ls
ln -s ./bin/minibox ./bin/kill
ln -s ./bin/minibox ./bin/sha1sum
ln -s ./bin/minibox ./bin/rdate
ln -s ./bin/minibox ./bin/sh
ln -s ./bin/minibox ./bin/true
ln -s ./bin/minibox ./bin/false
ln -s ./bin/minibox ./bin/lock
cd ..
