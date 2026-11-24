#!/usr/bin/env bash
# scripts/xvfb-run.sh <command> [arguments...]: runs a command on a headless desktop, for what needs a window or a GPU
# in a container that has neither: an X display (Xvfb) with a window manager (openbox), and Vulkan drawn in software
# (Mesa's lavapipe). FunkGui's GPU path on Linux is Vulkan on X11, so this is how its real drawing runs in the cloud;
# pictures made this way are the software Vulkan driver's, not a GPU's: say so where they are shown.
#
#   scripts/xvfb-run.sh vulkaninfo --summary
#   TEE_XVFB_SCREEN=2560x1440x24 scripts/xvfb-run.sh build-dev/bin/<a tool that opens a window>
#
# The display is this run's own (the first free number from 99), and Xvfb and openbox stop when the command ends; its
# exit status is the command's. scripts/bootstrap-cloud.sh installs what it needs.
set -euo pipefail
[ $# -gt 0 ] || { echo "usage: scripts/xvfb-run.sh <command> [arguments...]" >&2; exit 2; }
for tool in Xvfb openbox; do
  command -v "$tool" >/dev/null || { echo "xvfb-run: no $tool: run scripts/bootstrap-cloud.sh" >&2; exit 1; }
done

screen=${TEE_XVFB_SCREEN:-1920x1080x24}
log=$(mktemp -d "${TMPDIR:-/tmp}/tee-xvfb.XXXXXX")
display=99
while [ -e "/tmp/.X11-unix/X$display" ] || [ -e "/tmp/.X$display-lock" ]; do display=$((display + 1)); done

Xvfb ":$display" -screen 0 "$screen" -nolisten tcp > "$log/xvfb.log" 2>&1 &
xvfb=$!
wm=
cleanup() {
  [ -z "$wm" ] || kill "$wm" 2>/dev/null || true
  kill "$xvfb" 2>/dev/null || true
  wait 2>/dev/null || true
  rm -rf "$log"
}
trap cleanup EXIT
for _ in $(seq 100); do
  [ -S "/tmp/.X11-unix/X$display" ] && break
  kill -0 "$xvfb" 2>/dev/null || { echo "xvfb-run: Xvfb did not start:" >&2; cat "$log/xvfb.log" >&2; exit 1; }
  sleep 0.1
done
[ -S "/tmp/.X11-unix/X$display" ] || { echo "xvfb-run: Xvfb did not open display :$display" >&2; exit 1; }
export DISPLAY=":$display"
# (a runtime directory of this run's own, where the session has none: Vulkan's loader and SDL ask for one)
[ -n "${XDG_RUNTIME_DIR:-}" ] || export XDG_RUNTIME_DIR="$log"
openbox > "$log/openbox.log" 2>&1 &
wm=$!
sleep 0.5

# Mesa's software Vulkan (lavapipe), unless the caller chose a driver.
if [ -z "${VK_ICD_FILENAMES:-}" ] && [ -z "${VK_DRIVER_FILES:-}" ]; then
  for icd in /usr/share/vulkan/icd.d/lvp_icd.json /usr/share/vulkan/icd.d/lvp_icd.x86_64.json; do
    if [ -f "$icd" ]; then export VK_ICD_FILENAMES="$icd"; break; fi
  done
fi

rc=0
"$@" || rc=$?
exit $rc
