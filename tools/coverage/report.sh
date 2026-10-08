#!/bin/sh
# ++C Runtime Library
# Licensed under the MIT License

set -e

BUILD=${1:-build-cov}
MIN=${COVERAGE_MIN:-0}
OUT="$BUILD/coverage"

if [ ! -d "$BUILD" ]; then
    echo "report.sh: no such directory: $BUILD" >&2
    exit 2
fi

ROOT=$(pwd)
rm -rf "$OUT"
mkdir -p "$OUT"

GCDAS=$(find "$BUILD" -name '*.gcda' | sort)
if [ -z "$GCDAS" ]; then
    echo "report.sh: no .gcda files under $BUILD (did the tests run?)" >&2
    exit 2
fi

for f in $GCDAS; do
    #gcov -p -o "$(dirname "$f")" "$f" >/dev/null 2>&1 || true
    gcov -p -o "$(dirname "$f")" "$f"
done
mv ./*.gcov "$OUT"/ 2>/dev/null || true

awk -v root="$ROOT" -v out="$OUT/uncovered.txt" -v min="$MIN" '
function trim(s) { gsub(/^[ \t]+|[ \t]+$/, "", s); return s }

FNR == 1 { src = ""; excl = 0 }

{
    n = index($0, ":")
    if (n == 0) next
    count = trim(substr($0, 1, n - 1))
    rest  = substr($0, n + 1)
    m = index(rest, ":")
    if (m == 0) next
    lineno = trim(substr(rest, 1, m - 1)) + 0
    text   = substr(rest, m + 1)
    sub(/\r$/, "", text)

    if (lineno == 0) {
        if (substr(text, 1, 7) == "Source:") {
            src = substr(text, 8)
            sub(root "/", "", src)
        }
        next
    }
    if (src == "" || src ~ /^\// || src ~ /^(test|examples|tools)\//) next

    if (text ~ /GCOV_EXCL_START/) excl = 1
    skip = excl || (text ~ /GCOV_EXCL_LINE/)
    if (text ~ /GCOV_EXCL_STOP/) excl = 0
    if (skip) next

    if (count == "-") next
    key = src SUBSEP lineno
    if (!(key in seen)) { seen[key] = 1; files[src] = 1; body[key] = text }
    if (count !~ /^(#####|=====)/) hit[key] = 1
}

END {
    for (key in seen) {
        split(key, p, SUBSEP)
        tot[p[1]]++
        if (key in hit) cov[p[1]]++
        else miss[p[1]] = miss[p[1]] " " p[2]
    }
    T = H = 0
    for (f in files) {
        t = tot[f]; h = cov[f] + 0
        T += t; H += h
        printf "%6.1f%% %5d/%-5d %s\n", (t ? 100 * h / t : 100), h, t, f | "sort -n -k1,1 -k4"
    }
    close("sort -n -k1,1 -k4")

    printf "" > out
    for (f in files) {
        k = split(miss[f], ls, " ")
        for (i = 1; i <= k; i++) printf "%s:%d: %s\n", f, ls[i], body[f SUBSEP ls[i]] >> out
    }
    close(out)
    system("sort -t: -k1,1 -k2,2n -o " out " " out)

    pct = (T ? 100 * H / T : 100)
    printf "\nTOTAL  %.2f%%  (%d of %d lines, %d uncovered)\n", pct, H, T, T - H
    if (T - H > 0) printf "uncovered lines listed in %s\n", out
    if (pct + 0 < min + 0) {
        printf "FAIL: line coverage %.2f%% is below COVERAGE_MIN=%s\n", pct, min
        exit 1
    }
}
' "$OUT"/*.gcov