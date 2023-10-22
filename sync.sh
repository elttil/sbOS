#!/bin/sh
#pwd
#cd /home/anton/prj/osdev/
#cd ./userland/json/hashmap
#make clean
#make
#cd ../../..
#
#cd ./userland/json
#make clean
#make
#cd ../..
#
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
cd ./userland/nasm-2.16.01
#make clean
make
#make install
cd ../..

cd ./userland/compiler
make
cd ../..

#cd ./userland/init
#make clean
#make
#cd ../..
#
cd ./userland/sh
make clean
make
cd ../..
cd ./userland/sha1sum
make clean
make
cd ../..
#
#cd ./userland/cat
#make clean
#make
#cd ../..
#
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

#cd ./userland/figlet
#cd ./userland/figlet-2.2.5/
#make clean
#make
#cd ../..

pwd
sudo mount ext2.img mount
#sudo cp ./userland/init/init ./mount/init
#sudo cp ./userland/cat/cat ./mount/cat
sudo cp ./userland/sh/sh ./mount/sh
sudo cp ./userland/terminal/term ./mount/term
sudo cp ./userland/snake/snake ./mount/snake
sudo cp ./userland/ante/ante ./mount/ante
sudo cp ./userland/windowserver/ws ./mount/ws
sudo cp ./userland/test/test ./mount/test
sudo cp ./userland/minibox/minibox ./mount/minibox
sudo cp ./userland/nasm-2.16.01/nasm ./mount/nasm

sudo cp ./userland/figlet-2.2.5/figlet ./mount/
sudo cp ./userland/figlet-2.2.5/chkfont ./mount/
sudo cp ./userland/figlet-2.2.5/figlist ./mount/
sudo cp ./userland/figlet-2.2.5/showfigfonts ./mount/

sudo cp ./userland/figlet-2.2.5/fonts/*.flf ./mount/fonts/
sudo cp ./userland/figlet-2.2.5/fonts/*.flc ./mount/fonts/

sudo cp ./DOOM1.WAD ./mount/DOOM1.WAD
sudo cp ./userland/compiler/compiler ./mount/cc
sudo cp ./userland/sha1sum/sha1sum ./mount/sha1sum
sudo cp ./userland/sha1sum/sha1sum ./mount/sa
sudo cp ./userland/fasm/fasm ./mount/fasm
sudo cp ./userland/ed/ed ./mount/ed
sudo cp ./userland/SmallerC/smlrc ./mount/sc
sudo cp ./userland/libc/crt0.o ./mount/crt0.o
sudo cp ./userland/libc/libc.a ./mount/libc.a
sudo cp ./userland/doomgeneric/doomgeneric/doomgeneric ./mount/doomgeneric
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
