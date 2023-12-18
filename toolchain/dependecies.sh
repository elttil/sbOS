#!/bin/sh
# Automatically installs the dependecies for building the compiler.
DISTRO=$(uname -a | awk '{print $2}')
[ $DISTRO != "debian" ] && echo "Installer for dependecies only works on debian currently. Check https://wiki.osdev.org/GCC_Cross-Compiler#Installing_Dependencies for more information." && exit 1
# Currently only debian(11 or 12) is supported
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo
