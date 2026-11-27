#!/usr/bin/env bash
# scripts/run-wake.sh <TheEngineEngine> <tee-mcp>: the assistant's wake (app/AssistantWake.h) in the real application,
# its window minimised: the app is started with "Allow Claude Desktop" on (a settings folder of this run's own) and
# --minimise-after 30, and once SDL says the window is down, one tools/call (get_build) goes through tee-mcp, as Claude
# Desktop's would. It must be answered within 6 s while the window does not tick: the wake's drain on the main thread
# answers it; without the wake it waits for a frame that never comes (tee-mcp's own limit, 110 s).
#
#   scripts/xvfb-run.sh scripts/run-wake.sh build-dev/bin/TheEngineEngine build-dev/bin/tee-mcp     (Linux)
#
# One line says what was seen, for a reader of a CI log: what SDL said of the window, the frames ticked before the call
# and after the answer, and how long the answer took. Linux must pass (Xvfb with openbox minimises for real), and there
# the ticks must not move between the call and the answer: a pass with a ticking window would show nothing. Elsewhere
# (macOS's runner) the run reports. Where the window manager does not honour a minimise the app hides the window instead
# and the line says so. Windows: this version has no link there (no Claude Desktop extension on Windows yet): said, and
# a pass. Every folder the app reads or writes is this run's own (TEE_SETTINGS_DIR, TEE_USER_DIR, TEE_CACHE_DIR,
# TEE_PREFS_DIR, TEE_MCP_DIR): nothing of the person's. Portable to macOS's bash 3.2 and BSD tools: the clock is
# python3's (no date +%N, no bc).
set -euo pipefail
[ $# -eq 2 ] || { echo "usage: scripts/run-wake.sh <TheEngineEngine> <tee-mcp>" >&2; exit 2; }
app=$1
mcp=$2

case "$(uname -s)" in
  MINGW* | MSYS* | CYGWIN*) echo "run-wake: no link on this platform (Windows): nothing to call; not a failure"; exit 0 ;;
esac

work=$(mktemp -d "${TMPDIR:-/tmp}/tee-wake.XXXXXX")
pid=
cleanup() {
  [ -z "$pid" ] || kill "$pid" 2>/dev/null || true
  wait 2>/dev/null || true
  rm -rf "$work"
}
trap cleanup EXIT
mkdir -p "$work/settings" "$work/link" "$work/user" "$work/cache" "$work/prefs"
chmod 700 "$work/link"   # the link refuses a folder others may enter
# the settings file's version as this checkout's app reads it (shell/settings/Settings.h): run from the repository
here=$(cd "$(dirname "$0")/.." && pwd)
version=$(sed -nE 's/.*constexpr int kVersion = ([0-9]+);.*/\1/p' "$here/shell/settings/Settings.h" | head -1)
[ -n "$version" ] || { echo "run-wake: the settings version was not found in shell/settings/Settings.h" >&2; exit 1; }
printf '{ "format": "tee-settings", "version": %s, "allowClaudeDesktop": true }\n' "$version" > "$work/settings/settings.json"
# every folder the app reads or writes, this run's own: the settings, the builds and recordings (TEE_USER_DIR), the
# bakes' cache, FunkGui's preferences (the theme, the zoom: TEE_PREFS_DIR) and the link
export TEE_SETTINGS_DIR="$work/settings" TEE_USER_DIR="$work/user" TEE_CACHE_DIR="$work/cache" \
       TEE_PREFS_DIR="$work/prefs" TEE_MCP_DIR="$work/link"
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-dummy}"

"$app" --minimise-after 30 > "$work/app.log" 2>&1 &
pid=$!

# the window down (its first report) and the link open (at launch, before the simulator is ready), within a minute
for _ in $(seq 600); do
  grep -q '^minimise: [0-9.]* s ' "$work/app.log" && [ -S "$work/link/app.sock" ] && break
  kill -0 "$pid" 2>/dev/null || { echo "run-wake: the app ended before its window was down:" >&2; cat "$work/app.log" >&2; exit 1; }
  sleep 0.1
done
grep -q '^minimise: [0-9.]* s ' "$work/app.log" || { echo "run-wake: the window was never minimised:" >&2; cat "$work/app.log" >&2; exit 1; }
[ -S "$work/link/app.sock" ] || { echo "run-wake: the app's link never opened (Allow Claude Desktop):" >&2; cat "$work/app.log" >&2; exit 1; }
sleep 1   # a second down, ticks or none, before the call

last() { grep '^minimise: [0-9.]* s ' "$work/app.log" | tail -1; }
before=$(last)
ticksBefore=${before##*ticks=}
# the simulator as the call is sent: get_build reads only the document, so a call while it compiles must answer too
simulator=$(echo "$before" | sed -nE 's/.* simulator=([a-z]+) .*/\1/p')

# One call through tee-mcp, as Claude Desktop's: initialize, then tools/call get_build; stdin held open until its answer
# (6 s at most) so that tee-mcp waits for it, then let go. The answer, if any, in $work/mcp.$1.out.
ask() {
  local out="$work/mcp.$1.out"
  {
    printf '%s\n' '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18","capabilities":{},"clientInfo":{"name":"run-wake","version":"1"}}}'
    printf '%s\n' '{"jsonrpc":"2.0","method":"notifications/initialized"}'
    printf '%s\n' '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"get_build","arguments":{}}}'
    for _ in $(seq 60); do
      grep -q '"id":2' "$out" 2>/dev/null && break
      sleep 0.1
    done
  } | "$mcp" > "$out" 2> "$work/mcp.$1.err" &
  local feeder=$!
  for _ in $(seq 60); do
    grep -q '"id":2' "$out" 2>/dev/null && break
    sleep 0.1
  done
  wait "$feeder" 2>/dev/null || true
}

clock() { python3 -c 'import time; print("%.3f" % time.time())'; }
start=$(clock)
ask first
took=$(python3 -c "print('%.2f' % ($(clock) - $start))")
sleep 0.6   # one more report after the answer
after=$(last)
ticksAfter=${after##*ticks=}
flags=$(echo "$after" | sed -E 's/^minimise: [0-9.]+ s (.*) simulator=.*/\1/')
hidden=
grep -q 'not honoured here' "$work/app.log" && hidden=" (minimise not honoured: hidden instead)"

if ! grep -q '"id":2' "$work/mcp.first.out"; then
  echo "run-wake: FAILED: no answer within 6 s; window: $flags$hidden; ticks $ticksBefore before the call, $ticksAfter after; simulator ${simulator:-not said} at the call"
  echo "---- tee-mcp's stderr"; cat "$work/mcp.first.err"
  echo "---- the app's log, its end"; tail -n 40 "$work/app.log"
  exit 1
fi
if grep '"id":2' "$work/mcp.first.out" | grep -q '"isError":true'; then
  echo "run-wake: FAILED: the call answered with an error:"; grep '"id":2' "$work/mcp.first.out" | cut -c1-400
  exit 1
fi
# the property first (Linux: no tick between the call and the answer), then the line that says the run passed
if [ "$ticksBefore" != "$ticksAfter" ]; then
  if [ "$(uname -s)" = Linux ] && [ -z "$hidden" ]; then
    echo "run-wake: FAILED: the window ticked between the call and the answer ($ticksBefore, then $ticksAfter): minimised for real here, so this run shows nothing of the wake"
    exit 1
  fi
  echo "run-wake: note: the window ticked meanwhile, so this run does not show a drain without a frame (reported here, not required)"
fi
printf 'run-wake: answered in %s s; window: %s%s; ticks %s before the call, %s after the answer; simulator %s at the call\n' \
  "$took" "$flags" "$hidden" "$ticksBefore" "$ticksAfter" "${simulator:-not said}"

# Reported, not required (X-066): the simulator adopted while the window still does not tick. A minimised window adopts
# it only in a drain, so the script asks again about once a second until the app's line says simulator=ready, for 15 s
# at most (the app closes its window 20 s after the minimise): a compile slower than that is said, not failed.
for i in $(seq 15); do
  echo "$(last)" | grep -q ' simulator=ready ' && break
  ask "again$i"
  sleep 1
done
ready=$(last)
if echo "$ready" | grep -q ' simulator=ready '; then
  echo "run-wake: simulator ready while minimised, $(echo "$ready" | sed -E 's/^minimise: ([0-9.]+) s .*/\1/') s after the minimise; ticks ${ready##*ticks=} (reported)"
else
  echo "run-wake: note: the simulator was not ready within 15 s of asking (a slow compile; reported, not required); ticks ${ready##*ticks=}"
fi
