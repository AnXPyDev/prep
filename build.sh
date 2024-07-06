#!/bin/sh

CFLAGS="-O3 -Wall"

BUILD="build"
BINARY="$BUILD/prep"

init() {
    mkdir -p "$BUILD"
}

build() {
    cc src/main.c $CLFLAGS -o "$BINARY"
}

[ -z "$1" ] && exit 1
set -x
"$1"
