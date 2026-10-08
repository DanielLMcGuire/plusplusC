#!/bin/sh
set -e

BUILD=${1:-build-cov}
MIN=${COVERAGE_MIN:-0}
OUT="$BUILD/coverage"

if [ ! -d "$BUILD" ]; then
    echo "report.sh: no such directory: $BUILD" >&2
    exit 2
fi

ROOT=$(pwd)
EXCL=${COVERAGE_EXCLUSIONS:-tools/gcov/exclusions.txt}
[ -f "$EXCL" ] || EXCL=/dev/null
rm -rf "$OUT"
mkdir -p "$OUT"

GCDAS=$(find "$BUILD" -name '*.gcda' | sort)
if [ -z "$GCDAS" ]; then
    echo "report.sh: no .gcda files under $BUILD (did the tests run?)" >&2
    exit 2
fi

for f in $GCDAS; do
    gcov -p -o "$(dirname "$f")" "$f" >/dev/null 2>&1 || true
done
mv ./*.gcov "$OUT"/ 2>/dev/null || true

awk -v root="$ROOT" -v exfile="$EXCL" -v out="$OUT/uncovered.txt" -v min="$MIN" '
function trim(s) { gsub(/^[ \t]+|[ \t]+$/, "", s); return s }

# exclusions.txt:  FILE|SPEC|REASON   where SPEC is  N,  N-M  or  /regex/
BEGIN {
    while ((getline line < exfile) > 0) {
        if (line ~ /^[ \t]*(#|$)/) continue
        n = split(line, ef, "|")
        file = trim(ef[1]); spec = trim(ef[2])
        nex++
        ex_file[nex] = file; ex_spec[nex] = spec; ex_used[nex] = 0; ex_hit[nex] = 0
    }
    close(exfile)
}

function excluded(src, lineno, text,    i, a, b, sp, re) {
    for (i = 1; i <= nex; i++) {
        if (ex_file[i] != src) continue
        sp = ex_spec[i]
        if (sp ~ /^\/.*\/$/) {
            re = substr(sp, 2, length(sp) - 2)
            if (text ~ re) { ex_used[i]++; return i }
        } else if (index(sp, "-")) {
            split(sp, a, "-")
            if (lineno >= a[1] + 0 && lineno <= a[2] + 0) { ex_used[i]++; return i }
        } else if (lineno == sp + 0) { ex_used[i]++; return i }
    }
    return 0
}

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
    isHit = (count !~ /^(#####|=====)/)
    xi = excluded(src, lineno, text)
    if (xi) {
        if (isHit) ex_hit[xi]++
        if (!(key in xseen)) { xseen[key] = 1; nexcl++ }
        next
    }
    if (!(key in seen)) { seen[key] = 1; files[src] = 1; body[key] = text }
    if (isHit) hit[key] = 1
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

    for (i = 1; i <= nex; i++) {
        if (ex_used[i] == 0)
            printf "warning: stale exclusion (matches nothing): %s|%s\n", ex_file[i], ex_spec[i]
        else if (ex_hit[i] > 0)
            printf "warning: excluded line was executed, drop the exclusion: %s|%s\n", ex_file[i], ex_spec[i]
    }
    pct = (T ? 100 * H / T : 100)
    printf "\nTOTAL  %.2f%%  (%d of %d lines, %d uncovered)\n", pct, H, T, T - H
    if (nexcl > 0) printf "%d unreachable lines excluded (tools/gcov/exclusions.txt)\n", nexcl
    if (T - H > 0) printf "uncovered lines listed in %s\n", out
    if (pct + 0 < min + 0) {
        printf "FAIL: line coverage %.2f%% is below COVERAGE_MIN=%s\n", pct, min
        exit 1
    }
}
' "$OUT"/*.gcov
