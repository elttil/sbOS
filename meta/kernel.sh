#!/bin/sh
scriptdir="$(dirname "$0")"
cd "$scriptdir"
make -C ../kernel
