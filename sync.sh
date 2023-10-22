#!/bin/sh
cd ./userland/libgui
make clean
make
cd ../..
#
cd ./userland/libc
#make clean
make
make install
cd ../..

cd ./userland/sh
make clean
make
cd ../..
cd ./userland/terminal
make clean
make
cd ../..
#
cd ./userland/snake
make clean
make
cd ../..
#
cd ./userland/ante
make clean
make
cd ../..
#
cd ./userland/windowserver
make clean
make
cd ../..
#
cd ./userland/test
make clean
make
cd ../..
#
cd ./userland/minibox
make clean
make
cd ../..

pwd
sudo mount ext2.img mount
sudo cp ./userland/sh/sh ./mount/sh
sudo cp ./userland/terminal/term ./mount/term
sudo cp ./userland/snake/snake ./mount/snake
sudo cp ./userland/ante/ante ./mount/ante
sudo cp ./userland/windowserver/ws ./mount/ws
sudo cp ./userland/test/test ./mount/test
sudo cp ./userland/minibox/minibox ./mount/minibox

echo -e "int main(void) {\nprintf(\"hi\");\nreturn 0;\n}" > /tmp/main.c

cd ./mount
sudo rm ./init
sudo rm ./cat
sudo rm ./yes
sudo rm ./echo
sudo rm ./wc
sudo rm ./ls
sudo ln -s ./minibox ./init
sudo ln -s ./minibox ./cat
sudo ln -s ./minibox ./yes
sudo ln -s ./minibox ./echo
sudo ln -s ./minibox ./wc
sudo ln -s ./minibox ./ls
sudo cp /tmp/main.c ./main.c
cd ..
sudo umount mount
