#!/bin/sh
scriptdir="$(dirname "$0")"
cd "$scriptdir"
export PATH="$PATH:$(pwd)/../toolchain/bin/bin"
cd ..
make -C ./userland/libgui
make -C ./userland/libc
mkdir -p ./sysroot/lib
make install -C ./userland/libc
make -C ./userland/sh
make -C ./userland/terminal
make -C ./userland/snake
make -C ./userland/ante
make -C ./userland/windowserver
make -C ./userland/test
make -C ./userland/minibox
make -C ./userland/libppm
make -C ./userland/rtl8139
make -C ./userland/smol_http
make -C ./userland/irc

mkdir sysroot
sudo cp ./userland/rtl8139/rtl8139 ./sysroot/rtl8139
sudo cp ./userland/libppm/ppm ./sysroot/ppm
sudo cp ./userland/sh/sh ./sysroot/sh
sudo cp ./userland/terminal/term ./sysroot/term
sudo cp ./userland/snake/snake ./sysroot/snake
sudo cp ./userland/ante/ante ./sysroot/ante
sudo cp ./userland/windowserver/ws ./sysroot/ws
sudo cp ./userland/test/test ./sysroot/test
sudo cp ./userland/minibox/minibox ./sysroot/minibox
sudo cp ./userland/smol_http/smol_http ./sysroot/smol_http
sudo cp ./userland/irc/irc ./sysroot/irc

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
cd ..
