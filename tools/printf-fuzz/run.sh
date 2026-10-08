#!/bin/sh

set -e
cd "$(dirname "$0")/../.."
OUT=${TMPDIR:-/tmp}/xxc-printf-fuzz
mkdir -p "$OUT"

${CC:-gcc} -O1 -w -std=gnu2x -ffreestanding -fno-builtin -Dvsnprintf=xxc_vsnprintf \
    -Ilibminicrt/include -Iplatform -c libminicrt/src/format.c -o "$OUT/format.o"
${CC:-gcc} -O1 -w -std=gnu2x tools/printf-fuzz/fuzz.c "$OUT/format.o" -o "$OUT/fuzz" -lm

exec "$OUT/fuzz" "${1:-200000}" "${2:-1}"
