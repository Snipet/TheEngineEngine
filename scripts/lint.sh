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

if [ "$fail" -ne 0 ]; then
  echo "lint: the rules are at the top of scripts/lint.sh"
  exit 1
fi
echo "lint: ok (${#sources[@]} sources)"
