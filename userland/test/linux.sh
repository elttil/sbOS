#!/bin/bash
gcc -DLINUX test.c -o ./local/a.out
cd local
./a.out
