#!/usr/bin/env bash
# tests/sim/combo/spread.sh: a combo row's spread over temperature nudges, on demand and in no tier (the lead's ruling,
# 26 Nov 07:30: a click row is a sometimes row; this is how a cure is accepted). Every copy plays the row and its
# reference with env.temperature at 293.15 + dT (the pressure is no nudge: the engine never reads it, X-046), and prints
# per copy the excess over its firing periods, its time, the checks that failed, and the reference's own excess.
#
#   tests/sim/combo/spread.sh <case> <reference> [bound]
#     <case>, <reference>: tee-fuzz --play arguments, quoted: "<archetype|build.json> <scenario> <switches> [--set p=v]..."
#     bound: the row's bound (its judge's), for the count over; omitted, the count is not made
#   env: TEE_FUZZ (default build-dev/bin/tee-fuzz), TEE_SIM_DIR (the tree played; the source's sim/ when unset),
#        SPREAD_SIM_B (a second sim/ tree: every copy is played on both, the case and the reference each on its own
#        tree, and both columns are printed: a cure's before and after from one invocation), SPREAD_JOBS (default 4),
#        SPREAD_DT (the nudges in kelvin; default -0.15 to +0.15 in 17 steps of 0.02 with 0 among them)
#
# A corpus build's reference is the same build with every switch off: pass it with each switch --set to 0.
# The verdicts it supports (the lead's evidence rule): CURED (0 copies over, on the tree said to cure it, with the rate
# before on record) or NOT CURED; anything else is reported as "n of N, was m of N" with no verdict.
set -euo pipefail

if [ $# -lt 2 ]; then
    sed -n '2,19p' "$0" | sed 's/^# \{0,1\}//'
    exit 2
fi
case_args=$1
ref_args=$2
bound=${3:-}
fuzz=${TEE_FUZZ:-build-dev/bin/tee-fuzz}
jobs=${SPREAD_JOBS:-4}
dts=${SPREAD_DT:-"-0.15 -0.13 -0.11 -0.09 -0.07 -0.05 -0.03 -0.01 0 0.01 0.03 0.05 0.07 0.09 0.11 0.13 0.15"}
tree_a=${TEE_SIM_DIR:-}
tree_b=${SPREAD_SIM_B:-}
[ -x "$fuzz" ] || { echo "spread.sh: no tee-fuzz at $fuzz (set TEE_FUZZ)" >&2; exit 2; }
[ -z "$tree_b" ] || [ -d "$tree_b" ] || { echo "spread.sh: no tree at $tree_b (SPREAD_SIM_B)" >&2; exit 2; }

one() {   # <tree A|B> <which> <dT> <args>: prints tree which dT excess at checks
    local tree=$1 which=$2 dt=$3 args=$4 t out x c dir
    t=$(awk -v d="$dt" 'BEGIN { printf "%.4f", 293.15 + d }')
    if [ "$tree" = B ]; then dir=$tree_b; else dir=$tree_a; fi
    # shellcheck disable=SC2086
    out=$(TEE_SIM_DIR="$dir" "$fuzz" --play $args --set env.temperature="$t" 2>&1 || true)
    x=$(printf '%s\n' "$out" | grep -oE 'excess: [0-9.-]+ at [0-9.]+ s' | head -1 | awk '{ print $2, $4 }')
    c=$(printf '%s\n' "$out" | grep -oE '^  (ruled, )?[a-z-]+(:[a-zA-Z.]+)?:' | sed 's/^  //; s/:$//; s/ruled, /ruled-/' |
        grep -v '^excess$' | sort -u | paste -sd, - || true)
    echo "$tree $which $dt ${x:-n/a n/a} ${c:-none}"
}
export -f one
export fuzz tree_a tree_b

name_a=${tree_a:-"the source tree's sim/"}
trees="A"
[ -z "$tree_b" ] || trees="A B"
tmp=$(mktemp)
trap 'rm -f "$tmp"' EXIT
for tree in $trees; do
    for dt in $dts; do
        printf '%s\tcase\t%s\t%s\n' "$tree" "$dt" "$case_args"
        printf '%s\tref\t%s\t%s\n' "$tree" "$dt" "$ref_args"
    done
done | xargs -P "$jobs" -d '\n' -I{} bash -c 'IFS=$'"'"'\t'"'"' read -r tr w d a <<< "$1"; one "$tr" "$w" "$d" "$a"' _ {} > "$tmp"

for tree in $trees; do
    if [ "$tree" = A ]; then
        echo "== tree A: $name_a"
    else
        echo "== tree B: $tree_b"
    fi
    awk -v bound="$bound" -v tree="$tree" '
        $1 != tree { next }
        { key = $3 + 0; if ($2 == "case") { ex[key] = $4; at[key] = $5; ck[key] = $6 } else { rx[key] = $4 } ; ds[key] = $3 }
        END {
            n = 0; for (k in ds) keys[++n] = k + 0
            for (i = 1; i <= n; ++i) for (j = i + 1; j <= n; ++j) if (keys[j] < keys[i]) { t = keys[i]; keys[i] = keys[j]; keys[j] = t }
            printf "%8s  %8s  %9s  %9s  %s\n", "dT (K)", "excess", "at (s)", "reference", "checks failed"
            over = 0; m = 0; rm = 0
            for (i = 1; i <= n; ++i) {
                k = keys[i]
                printf "%+8.2f  %8s  %9s  %9s  %s\n", k, ex[k], at[k], rx[k], ck[k]
                if (ex[k] != "n/a") { v[++m] = ex[k] + 0; if (bound != "" && ex[k] + 0 > bound + 0) ++over }
                if (rx[k] != "n/a") r[++rm] = rx[k] + 0
            }
            for (i = 1; i <= m; ++i) for (j = i + 1; j <= m; ++j) if (v[j] < v[i]) { t = v[i]; v[i] = v[j]; v[j] = t }
            for (i = 1; i <= rm; ++i) for (j = i + 1; j <= rm; ++j) if (r[j] < r[i]) { t = r[i]; r[i] = r[j]; r[j] = t }
            if (m > 0) printf "case: lowest %.2f, median %.2f, highest %.2f", v[1], v[int((m + 1) / 2)], v[m]
            if (bound != "") printf "; %d of %d copies over %s", over, m, bound
            printf "\n"
            if (rm > 0) printf "reference: lowest %.2f, median %.2f, highest %.2f\n", r[1], r[int((rm + 1) / 2)], r[rm]
        }' "$tmp"
done
