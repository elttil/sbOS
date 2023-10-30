#!/bin/sh
scriptdir="$(dirname "$0")"
cd "$scriptdir"
export PATH="$PATH:$(pwd)/../toolchain/bin/bin"
make -C ../kernel
