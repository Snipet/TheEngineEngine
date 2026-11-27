#!/usr/bin/env bash
# scripts/run-wake.sh <TheEngineEngine> <tee-mcp>: the assistant's wake (app/AssistantWake.h) in the real application,
# its window minimised: the app is started with "Allow Claude Desktop" on (a settings folder of this run's own) and
# --minimise-after 30, and once SDL says the window is down, one tools/call (get_build) goes through tee-mcp, as Claude
# Desktop's would. It must be answered within 5 s while the window does not tick: the wake's drain on the main thread
# answers it; without the wake it waits for a frame that never comes (tee-mcp's own limit, 110 s).
#
#   scripts/xvfb-run.sh scripts/run-wake.sh build-dev/bin/TheEngineEngine build-dev/bin/tee-mcp     (Linux)
#
# One line says what was seen, for a reader of a CI log: what SDL said of the window, the frames ticked before the call
# and after the answer, and how long the answer took. Linux must pass (Xvfb with openbox minimises for real). Where the
# window manager does not honour a minimise the app hides the window instead and the line says so. Windows: this
# version has no link there (no Claude Desktop extension on Windows yet): said, and a pass.
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
mkdir -p "$work/settings" "$work/link"
chmod 700 "$work/link"   # the link refuses a folder others may enter
# the settings file's version as this checkout's app reads it (shell/settings/Settings.h): run from the repository
here=$(cd "$(dirname "$0")/.." && pwd)
version=$(sed -nE 's/.*constexpr int kVersion = ([0-9]+);.*/\1/p' "$here/shell/settings/Settings.h" | head -1)
[ -n "$version" ] || { echo "run-wake: the settings version was not found in shell/settings/Settings.h" >&2; exit 1; }
printf '{ "format": "tee-settings", "version": %s, "allowClaudeDesktop": true }\n' "$version" > "$work/settings/settings.json"
export TEE_SETTINGS_DIR="$work/settings" TEE_MCP_DIR="$work/link"
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-dummy}"

"$app" --minimise-after 30 > "$work/app.log" 2>&1 &
pid=$!

# the window down (its first report) and the link open, within a minute (the simulator's compile comes first)
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

# initialize, then the call; stdin held open 6 s so that tee-mcp waits for the answer, then let go
start=$(date +%s.%N)
{
  printf '%s\n' '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18","capabilities":{},"clientInfo":{"name":"run-wake","version":"1"}}}'
  printf '%s\n' '{"jsonrpc":"2.0","method":"notifications/initialized"}'
  printf '%s\n' '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"get_build","arguments":{}}}'
  for _ in $(seq 60); do
    grep -q '"id":2' "$work/mcp.out" 2>/dev/null && break
    sleep 0.1
  done
} | "$mcp" > "$work/mcp.out" 2> "$work/mcp.err" &
feeder=$!
for _ in $(seq 60); do
  grep -q '"id":2' "$work/mcp.out" 2>/dev/null && break
  sleep 0.1
done
took=$(echo "$(date +%s.%N) - $start" | bc)
wait "$feeder" 2>/dev/null || true
sleep 0.6   # one more report after the answer
after=$(last)
ticksAfter=${after##*ticks=}
flags=$(echo "$after" | sed -E 's/^minimise: [0-9.]+ s ([^t]*)ticks=.*/\1/; s/ +$//')
hidden=
grep -q 'not honoured here' "$work/app.log" && hidden=" (minimise not honoured: hidden instead)"

if ! grep -q '"id":2' "$work/mcp.out"; then
  echo "run-wake: FAILED: no answer within 6 s; window: $flags$hidden; ticks $ticksBefore before the call, $ticksAfter after"
  echo "---- tee-mcp's stderr"; cat "$work/mcp.err"
  exit 1
fi
if grep '"id":2' "$work/mcp.out" | grep -q '"isError":true'; then
  echo "run-wake: FAILED: the call answered with an error:"; grep '"id":2' "$work/mcp.out" | cut -c1-400
  exit 1
fi
printf 'run-wake: answered in %.2f s; window: %s%s; ticks %s before the call, %s after the answer\n' \
  "$took" "$flags" "$hidden" "$ticksBefore" "$ticksAfter"
if [ "$ticksBefore" != "$ticksAfter" ]; then
  echo "run-wake: note: the window ticked meanwhile, so this run does not show a drain without a frame"
fi
