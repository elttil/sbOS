#!/bin/sh
scriptdir="$(dirname "$0")"
cd "$scriptdir"
cd ..
gdb -x .gdbinit
