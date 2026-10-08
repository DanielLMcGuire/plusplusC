#!/bin/sh
# ++C Runtime Library
# Licensed under the MIT License
set -e

EFI=${1:-out/BOOTX64.EFI}
IMG=${2:-out/disk.img}
SIZE=${SIZE_MB:-64}

dd if=/dev/zero of=$IMG bs=1M count=$SIZE status=none
sgdisk -o -n 1:2048:0 -t 1:EF00 $IMG >/dev/null

mformat -i $IMG@@1M -F -h 32 -t 32 -n 64 ::
mmd -i $IMG@@1M ::/EFI ::/EFI/BOOT

mcopy -i $IMG@@1M $EFI ::/EFI/BOOT/BOOTX64.EFI

if [ -n "$EXTRA" ]; then
    for item in $EXTRA; do
        SRC="${item%%:*}"
        DST="${item##*:}"
        if [ -f "$SRC" ]; then
            echo "copying $SRC -> ::/$DST"
            mcopy -i $IMG@@1M "$SRC" "::/$DST"
        fi
    done
fi

echo "wrote $IMG (${SIZE}MB)"