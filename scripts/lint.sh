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
#   4. no static that owns a compiled program: a `static` whose type holds a tee::Program, Instance, Sim,
#      EnginePlayer, RefPlayer or SimHost by value or through a smart pointer or a container is destroyed at exit
#      after FMinus's and LLVM's own statics on macOS, and its destructor runs on a JIT that is gone (38 tests crashed
#      so; docs/plan/01-rules.md §4). A raw pointer that is never deleted is the way (a `static Program*`), or the test
#      library's never-destroyed cache. A struct that holds one by a name of its own is not seen: mind it.
#   5. the Spec's lint (tee-schema --lint, host/src/Fields.cpp; 01-rules.md §4): every field of sim/engine.fm has its
#      label, range, default and a ui::unit, ui::quantity or ui::choices; a map its value's. Run when a build tree has
#      tee-schema (it compiles sim/ as it is now: 5 s); skipped, and said, when none has. TEE_LINT_SCHEMA=0 skips it.
#      It fails when sim/ does not compile at all (a merge that broke the program: the front end's error is printed).
#   6. the C++ that is about to be pushed compiles (scripts/syntax-check.py, -fsyntax-only, 2 s): the host's public
#      headers in one unit, and each .cpp that differs from the integration branch, with the newest build tree's
#      compile_commands.json. Skipped, and said, without a build tree. TEE_LINT_SYNTAX=0 skips it.
#   7. every telemetry field is inside its range from the first block (tee-tests-combo's FirstBlockTest: the default
#      V8 for half a second, every switch off and every switch on, judged as the combos judge: 8 s). A new field with
#      no sane first value turned 65 combo cases red on every platform twice on 2026-11-24. Skipped, and said, when no
#      build tree has a tee-tests-combo that holds the case. TEE_LINT_TELEMETRY=0 skips it.
#   9. a test tagged [nightly] is in a package whose catch_discover_tests has ADD_TAGS_AS_LABELS (else ctest sees no
#      label and the full form runs in the fast tier).
#   8. no float becomes an array index without a clamp at both ends (scripts/lint-index.py: a NaN's cast is INT_MIN on
#      x86, X-019's SIGSEGV): tableIndex(x, last) (sim/core/mathx.fm), fminf(fmaxf(...)), or a lower check, or the line
#      says `// lint: index-ok: <why>`. The places known on 2026-11-24 are in scripts/lint-index.allow, reported.
#  10. a file under tests/ or tools/ that calls tee::loadBuild or parseBuild reads its `warnings` (beyond declaring
#      it and passing it): the loader drops what it cannot take there (a nested ecu.limiter, ecu.idle, ecu.knock), and
#      until 2026-11-24 the rigs ran 62 archetypes without their own idle and limiter so. Since C's one strict loader
#      (127c350) a field the program lacks is an error by default, and `warnings` holds only values out of their range:
#      a note, unless the call passes strict = false (the field is then kept and only `warnings` says so): a failure.
#
# Our files are the ones git knows (the dependencies are not in the checkout).
set -euo pipefail
cd "$(dirname "$0")/.."

fail=0
say() { echo "lint: $*"; fail=1; }

# ---- 0. nothing generated is tracked: Python's bytecode ------------------------------------------------------------
while IFS= read -r f; do
  [ -n "$f" ] && say "$f: Python bytecode is tracked (generated: git rm --cached it; .gitignore has __pycache__/ and *.pyc)"
done < <(git ls-files -- '*.pyc' '*/__pycache__/*' '__pycache__/*')

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

# ---- 4. a static that owns a compiled program ------------------------------------------------------------------------
owner='\bstatic\s+[^(;=]*\b(Program|Instance|Sim|EnginePlayer|RefPlayer|SimHost)\b[^(;=*]*\s[A-Za-z_][A-Za-z0-9_]*\s*(=|;|\{)'
for f in "${sources[@]}"; do
  [ -f "$f" ] || continue
  grep -qE '\bstatic\b' "$f" || continue
  while IFS=: read -r line _; do
    say "$f:$line: a static that owns a compiled program (destroyed after the JIT at exit on macOS): a never-deleted pointer instead"
  done < <(code "$f" | grep -nE "$owner")
done

# ---- a build tree configured before the files its configure reads (a warning, not a failure) -------------------------
# Test names, workspaces and packages are found at configure time (globs, VIEW_TEST lines, register.h): a tree whose
# CMake has not re-run since a merge runs an old list against new files, and its results mislead (a whole suite's
# count once). `cmake --build` re-runs CMake for most of them; `ctest` alone never does.
for tree in build-*/; do
  [ -f "$tree/CMakeCache.txt" ] || continue
  stamp="$tree/CMakeCache.txt"
  [ -f "$tree/build.ninja" ] && stamp="$tree/build.ninja"
  newer=$(git ls-files -- '*CMakeLists.txt' '*.cmake' '*/register.h' 'tests/ui/views/*.cpp' 'CMakePresets.json' \
          | while IFS= read -r f; do if [ "$f" -nt "$stamp" ]; then echo "$f"; fi; done | head -3 || true)
  if [ -n "$newer" ]; then
    echo "lint: warning: ${tree%/} was configured before $(echo "$newer" | paste -sd, -) changed:" \
         "run cmake --preset $(basename "${tree#build-}") (or cmake --build) before trusting its tests"
  fi
done

# ---- 5. the Spec's lint ---------------------------------------------------------------------------------------------
if [ "${TEE_LINT_SCHEMA:-1}" != 0 ] && [ -f sim/engine.fm ]; then
  schema=$(ls -t build-*/bin/tee-schema build-*/bin/tee-schema.exe 2>/dev/null | head -1 || true)
  if [ -n "$schema" ]; then
    jsonfile=$(mktemp); trap 'rm -f "$jsonfile"' EXIT
    "$schema" --sim sim --json > "$jsonfile" 2> /dev/null & jsonpid=$!   # beside the lint: each compiles sim/ (6 s)
    if ! out=$("$schema" --sim sim --lint 2>&1); then
      while IFS= read -r p; do [ -n "$p" ] && say "sim/engine.fm: $p"; done <<< "$out"
    fi
    # reported, not a failure: the visible Spec fields with no ui::help (the manual's table fills them:
    # docs/manual/help.tsv, then scripts/apply-help.py)
    if command -v python3 > /dev/null && wait "$jsonpid" && json=$(cat "$jsonfile") && [ -n "$json" ]; then
      python3 -c '
import json, sys
d = json.loads(sys.stdin.read())
bare = [f["path"] for f in d["fields"] if f["path"].startswith("spec.") and f.get("kind") != "rev"
        and not f.get("hidden") and not f.get("help")]
if bare:
    more = ", ..." if len(bare) > 5 else ""
    print("lint: note: %d visible Spec fields have no ui::help (%s%s): docs/manual/help.tsv, then "
          "scripts/apply-help.py" % (len(bare), ", ".join(bare[:5]), more))' <<< "$json"
    fi
  else
    echo "lint: no build tree with tee-schema: the Spec's lint skipped (cmake --build --preset dev --target tee-schema)"
  fi
fi

# ---- 6. the C++ about to be pushed compiles ---------------------------------------------------------------------------
if [ "${TEE_LINT_SYNTAX:-1}" != 0 ] && command -v python3 > /dev/null; then
  cc=$(ls -t build-*/compile_commands.json 2>/dev/null | head -1 || true)
  if [ -n "$cc" ]; then
    upstream=$(git rev-parse --verify -q '@{upstream}' 2>/dev/null || git rev-parse --verify -q origin/claude/awesome-gates-ntdb58 2>/dev/null || true)
    changed=()
    if [ -n "$upstream" ]; then
      mapfile -t changed < <(git diff --name-only --diff-filter=AM "$upstream" HEAD -- '*.cpp' '*.cc' 2>/dev/null | head -40)
    fi
    if ! out=$(python3 scripts/syntax-check.py "$(dirname "$cc")" "${changed[@]}" 2>/dev/null); then
      while IFS= read -r l; do [ -n "$l" ] && say "${l#syntax: }"; done <<< "$out"
    fi
  else
    echo "lint: no build tree with compile_commands.json: the C++ syntax check skipped"
  fi
fi

# ---- 9. a [nightly] tag is a ctest label: its package registers with ADD_TAGS_AS_LABELS ----------------------------
# (without it the tag is a Catch tag only, ctest sees no label, and the full form runs in the fast tier: ecu and
# combustion did so on 2026-11-24, 300 CPU-s between them)
while IFS= read -r cm; do
  d=$(dirname "$cm")
  if grep -lq '\[nightly\]' "$d"/*.cpp 2>/dev/null && ! grep -q ADD_TAGS_AS_LABELS "$cm"; then
    say "$cm: its tests have [nightly] tags but catch_discover_tests has no ADD_TAGS_AS_LABELS: they run in the fast tier"
  fi
done < <(git ls-files -- 'tests/*CMakeLists.txt')

# ---- 10. a rig that loads a build reads what the loader dropped ------------------------------------------------------
# tee::loadBuild / parseBuild put in `warnings` what they kept without taking (strict = false) and values out of range;
# a non-strict caller that never reads it measures another engine (rule text at the top).
while IFS= read -r f; do
  [ -n "$f" ] || continue
  reads=$(grep -vE '(loadBuild|parseBuild)[[:space:]]*\(|^[[:space:]]*std::string[^;(]*warnings[^;(]*;' "$f" | grep -c 'warnings' || true)
  if [ "${reads:-0}" = 0 ]; then   # (no grep -q in a pipe: under pipefail its early exit fails the pipe)
    if grep -qE '(loadBuild|parseBuild)[[:space:]]*\([^;]*,[[:space:]]*false[[:space:]]*\)' "$f"; then
      say "$f: calls loadBuild/parseBuild with strict = false and never reads its warnings (the fields the program lacks): fail on them, or print them"
    else
      echo "lint: note: $f: never reads loadBuild/parseBuild's warnings (values out of their range): print them"
    fi
  fi
done < <(git grep -lE '(loadBuild|parseBuild)[[:space:]]*\(' -- 'tests/*' 'tools/*' || true)

# ---- 8. a float as an index, clamped at both ends --------------------------------------------------------------------
if command -v python3 > /dev/null; then
  if ! out=$(python3 scripts/lint-index.py 2>&1); then
    while IFS= read -r l; do
      case "$l" in known*) ;; "") ;; *) say "$l: a float as an index without a clamp at both ends (tableIndex, sim/core/mathx.fm)" ;; esac
    done <<< "$out"
  fi
  n=$(grep -c '^known' <<< "$out" || true)
  [ "$n" = 0 ] || echo "lint: note: $n known places use a float as an index unclamped (scripts/lint-index.allow): their owners have them"
fi

# ---- 7. telemetry in range from the first block ----------------------------------------------------------------------
if [ "${TEE_LINT_TELEMETRY:-1}" != 0 ]; then
  combo=$(ls -t build-*/bin/tee-tests-combo build-*/bin/tee-tests-combo.exe 2>/dev/null | head -1 || true)
  case_name='telemetry: every field inside its range from the first block\, every switch off and every switch on'
  if [ -n "$combo" ]; then
    if ! out=$("$combo" "$case_name" 2>&1); then
      if grep -q 'No test cases matched\|No tests ran' <<< "$out"; then
        echo "lint: $combo has no first-block telemetry case yet (rebuild tee-tests-combo): skipped"
      else
        while IFS= read -r l; do say "telemetry: ${l# *}"; done < <(grep -E 'tel-range|tel-finite' <<< "$out" | sed 's/^ *//' | sort -u)
        grep -qE 'tel-range|tel-finite' <<< "$out" || say "telemetry: the first-block case failed ($combo): run it to see why"
      fi
    fi
  else
    echo "lint: no build tree with tee-tests-combo: the first-block telemetry check skipped"
  fi
fi

if [ "$fail" -ne 0 ]; then
  echo "lint: the rules are at the top of scripts/lint.sh"
  exit 1
fi
echo "lint: ok (${#sources[@]} sources)"
