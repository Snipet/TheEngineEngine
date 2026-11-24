#!/usr/bin/env bash
# scripts/lint.sh: the platform rules a Linux build cannot see, checked by grep in a second, before CI's macOS and
# Windows jobs find them twenty minutes later. Run by CI's linux-x64 job first; run it before you push.
#
#   scripts/lint.sh          # exit 1 and say where, if any rule is broken
#
# The rules (each failed on CI once):
#   1. (not here: the configure checks it, cmake/TeeChecks.cmake) every target of ours gets tee_target_defaults(),
#      which holds the Windows runtime's _USE_MATH_DEFINES (M_PI) and NOMINMAX.
#   2. no floating-point std::from_chars: Apple's libc++ marks it unavailable before macOS 26 and the build stops
#      there. Use strtod / strtof (in the "C" locale) or std::stod. An integer from_chars is fine: mark its line
#      with "lint: integer".
#   3. POSIX-only calls (clock_gettime and its thread clock, <unistd.h>, <pthread.h>, <dlfcn.h>, <sys/time.h>,
#      <sys/resource.h>, getrusage) only in a file that also says _WIN32, i.e. one that has the Windows branch.
#      For the thread's processor time use tee::threadCpuSeconds() (tee/Clock.h).
#
# Our files are the ones git knows (the dependencies are not in the checkout).
set -euo pipefail
cd "$(dirname "$0")/.."

fail=0
say() { echo "lint: $*"; fail=1; }

# ---- C and C++ sources of ours ----------------------------------------------------------------------------------------
mapfile -t sources < <(git ls-files -- '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hpp' '*.inc' '*.mm')

# lines of code only: // comments dropped (enough for these rules; a rule in a /* */ comment is rare)
code() { sed -E 's@//.*$@@' "$1"; }

# ---- 2. floating-point std::from_chars --------------------------------------------------------------------------------
for f in "${sources[@]}"; do
  [ -f "$f" ] || continue
  grep -q 'from_chars' "$f" || continue
  while IFS=: read -r line text; do
    [[ "$text" == *'lint: integer'* ]] && continue
    say "$f:$line: std::from_chars: a floating-point one does not build on macOS (use strtod), an integer one says \"// lint: integer\""
  done < <(grep -n 'from_chars' "$f" | grep -v -E '^[0-9]+:\s*//' )
done

# ---- 3. POSIX-only calls without a Windows branch ---------------------------------------------------------------------
posix='\bclock_gettime\b|CLOCK_(THREAD|PROCESS)_CPUTIME_ID|\bgetrusage\b|#\s*include\s*<(unistd|pthread|dlfcn|sys/time|sys/resource)\.h>'
for f in "${sources[@]}"; do
  [ -f "$f" ] || continue
  grep -qE "$posix" "$f" || continue
  grep -q '_WIN32' "$f" && continue
  code "$f" | grep -nE "$posix" | head -1 | while IFS=: read -r line _; do
    echo "lint: $f:$line: a POSIX-only call in a file with no _WIN32 branch (for processor time: tee::threadCpuSeconds, tee/Clock.h)"
  done | grep . && fail=1 || true
done

if [ "$fail" -ne 0 ]; then
  echo "lint: the rules are at the top of scripts/lint.sh"
  exit 1
fi
echo "lint: ok (${#sources[@]} sources)"
