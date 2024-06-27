#!/bin/bash
gcc -DLINUX test.c -g -o ./local/a.out || exit
cd local
valgrind ./a.out
