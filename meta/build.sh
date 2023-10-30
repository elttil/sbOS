#!/bin/sh
scriptdir="$(dirname "$0")"
cd "$scriptdir"
./kernel.sh && ./userland.sh && ./sync.sh
