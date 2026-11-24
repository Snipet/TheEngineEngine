#!/usr/bin/env bash
# scripts/test-budget.sh <ctest log>: the fast tier's time by package against its budget (docs/plan/01-rules.md §3),
# as a Markdown table: each package's test-seconds (the sum of its tests' times as ctest prints them, ctest's sec*proc; not time(1)'s processor time, which counts the JIT's threads), its budget, and
# whether it is over; then the whole tier against its own. CI's fast tier writes it into each job's summary; run it on
# any `ctest --preset dev` log (`ctest --preset dev | tee ctest.log`). It reports; it never fails.
#
# The budgets are the whole fast tier's cap of 2400 CPU-s on CI's Linux runner (ten minutes of wall on four cores),
# shared out by the lead's weights scaled by 2400 / 4500: null 580 (the guardian, kept whole), diesel,
# exhaust and the host 135, vehicle 100 (20 of null's, 2026-11-24), every other package 80, and combo what is left (120). What does not fit goes to the soak
# tier ([nightly]), by the owner's choice of what guards most per second.
# A hosted runner is one of several machines, up to 1.4 times apart (2026-11-24: a Xeon 6975P-C and a Xeon Platinum
# 8573C ran the same tests 1.37 times apart). Given tee-bench --quick's table of the same job as a second argument, the
# times are also scaled to the reference runner (the 6975P-C, whose -O2 compile of the reference took 0.93 s
# together) by that job's own -O2 compile time, and the verdict is on the scaled ones:
#   scripts/test-budget.sh ctest.log bench-quick.md
# A package is a test's prefix before its first "." or ":" (ctest names: "combo.", "core: "); a test with none is the
# host's (tee-tests, tee-host-tests).
set -euo pipefail
log=${1:?usage: scripts/test-budget.sh <ctest log> [tee-bench --quick table]}
scale=1
if [ -n "${2:-}" ] && [ -f "$2" ]; then
  # the -O2 row's "together" (the fourth cell) against the reference runner's 0.93 s
  scale=$(awk -F'|' '$2 ~ /-O2/ { gsub(/ /, "", $5); if ($5 + 0 > 0) printf "%.3f", $5 / 0.93; exit }' "$2")
  [ -n "$scale" ] || scale=1
fi
sed -nE 's/^ *[0-9]+\/ *[0-9]+ Test +#[0-9]+: (.*[^ .]) \.+ *([A-Za-z*]+.*) +([0-9.]+) sec$/\3\t\1/p' "$log" |
awk -F'\t' -v scale="$scale" '
  BEGIN { b["combo"] = 120; b["null"] = 580; b["vehicle"] = 100; b["diesel"] = 135; b["exhaust"] = 135; b["host"] = 135; other = 80; whole = 2400 }
  {
    p = $2
    if (match(p, /^[A-Za-z0-9-]+[.:]/)) p = substr(p, 1, RLENGTH - 1); else p = "host"
    s[p] += $1; n[p]++; tot += $1
  }
  END {
    if (scale != 1) {
      printf "This runner is %.2f times the reference runner (tee-bench -O2 compile): the verdict is on the scaled time.\n\n", scale
      print "| package | tests | test-s | scaled | budget | |"
      print "|---|---:|---:|---:|---:|---|"
    } else {
      print "| package | tests | test-s | budget | |"
      print "|---|---:|---:|---:|---|"
    }
    cmd = (scale != 1) ? "sort -t\"|\" -k5 -rn" : "sort -t\"|\" -k4 -rn"
    for (k in s) {
      bud = (k in b) ? b[k] : other
      v = s[k] / scale
      if (scale != 1) printf "| %s | %d | %.0f | %.0f | %d | %s |\n", k, n[k], s[k], v, bud, (v > bud ? "**over**" : "") | cmd
      else printf "| %s | %d | %.0f | %d | %s |\n", k, n[k], s[k], bud, (s[k] > bud ? "**over**" : "") | cmd
    }
    close(cmd)
    t = tot / scale
    if (scale != 1) printf "\nThe whole fast tier: %.0f test-s here, %.0f scaled, against %d%s\n", tot, t, whole, (t > whole ? " (**over**)" : "")
    else printf "\nThe whole fast tier: %.0f test-s against %d%s\n", tot, whole, (tot > whole ? " (**over**)" : "")
  }'
