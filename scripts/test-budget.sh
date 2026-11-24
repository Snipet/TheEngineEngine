#!/usr/bin/env bash
# scripts/test-budget.sh <ctest log>: the fast tier's time by package against its budget (docs/plan/01-rules.md §3),
# as a Markdown table: each package's CPU-seconds (the sum of its tests' times, as ctest prints them), its budget, and
# whether it is over; then the whole tier against its own. CI's fast tier writes it into each job's summary; run it on
# any `ctest --preset dev` log (`ctest --preset dev | tee ctest.log`). It reports; it never fails.
#
# The budgets are the whole fast tier's cap of 2400 CPU-s on CI's Linux runner (ten minutes of wall on four cores),
# shared out by the lead's weights scaled by 2400 / 4500: null 600 (the guardian, kept whole: it measured 590), diesel,
# exhaust and the host 135, every other package 80, and combo what is left (120). What does not fit goes to the soak
# tier ([nightly]), by the owner's choice of what guards most per second.
# A package is a test's prefix before its first "." or ":" (ctest names: "combo.", "core: "); a test with none is the
# host's (tee-tests, tee-host-tests).
set -euo pipefail
log=${1:?usage: scripts/test-budget.sh <ctest log>}
sed -nE 's/^ *[0-9]+\/ *[0-9]+ Test +#[0-9]+: (.*[^ .]) \.+ *([A-Za-z*]+.*) +([0-9.]+) sec$/\3\t\1/p' "$log" |
awk -F'\t' '
  BEGIN { b["combo"] = 120; b["null"] = 600; b["diesel"] = 135; b["exhaust"] = 135; b["host"] = 135; other = 80; whole = 2400 }
  {
    p = $2
    if (match(p, /^[A-Za-z0-9-]+[.:]/)) p = substr(p, 1, RLENGTH - 1); else p = "host"
    s[p] += $1; n[p]++; tot += $1
  }
  END {
    print "| package | tests | CPU-s | budget | |"
    print "|---|---:|---:|---:|---|"
    cmd = "sort -t\"|\" -k4 -rn"
    for (k in s) {
      bud = (k in b) ? b[k] : other
      printf "| %s | %d | %.0f | %d | %s |\n", k, n[k], s[k], bud, (s[k] > bud ? "**over**" : "") | cmd
    }
    close(cmd)
    printf "\nThe whole fast tier: %.0f CPU-s against %d%s\n", tot, whole, (tot > whole ? " (**over**)" : "")
  }'
