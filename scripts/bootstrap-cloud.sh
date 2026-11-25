#!/usr/bin/env bash
# scripts/bootstrap-cloud.sh: makes a fresh cloud container (Ubuntu 24.04, x86-64) able to build TheEngineEngine.
# Run it once; run again, it finds everything in place and changes nothing.
#
#   scripts/bootstrap-cloud.sh
#   cmake --workflow --preset dev-verify        # configure, build, run every test
#
# What it does:
#   1. apt packages: Ninja, clang and lld, clang's sanitizer runtimes (the tsan preset), Flex and Bison (FMinus's
#      grammar), ccache, and what FunkGui's GPU path and SDL3 need to build and to run without a screen (X11 and
#      Vulkan development packages, Xvfb, Mesa's software Vulkan, openbox: scripts/xvfb-run.sh), and libcurl and
#      libsecret for card A's assistant (its HTTPS transport and its key in the desktop's secret store; without them
#      those two parts compile off and say so).
#   2. CMake 3.30 or newer (Ubuntu 24.04 has 3.28): pip's, when the one on the PATH is older.
#   3. FMinus's prebuilt LLVM, checked against its release's SHA-256, into $TEE_DEPS_DIR/install (scripts/fetch-llvm.sh).
#   4. Every dependency's source, cloned by git at its pinned commit into $TEE_DEPS_DIR/src (cmake/TeeFetch.cmake):
#      a cloud container refuses release archives by URL, which is how FMinus fetches four of its own.
#
# TEE_DEPS_DIR is the dependency cache, shared by every checkout on the machine: /home/user/.tee-deps unless the
# environment says otherwise (cmake/TeeDeps.cmake finds that one by itself; another needs TEE_DEPS_DIR exported when
# configuring too). Never delete it, and never write into it by hand.
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
export TEE_DEPS_DIR=${TEE_DEPS_DIR:-/home/user/.tee-deps}
mkdir -p "$TEE_DEPS_DIR"
say() { printf '\n== %s\n' "$*"; }

# ---- 1. apt ----------------------------------------------------------------------------------------------------------
packages=(
  git curl ca-certificates xz-utils pkg-config python3-pip
  ninja-build clang lld libclang-rt-18-dev flex bison ccache
  zlib1g-dev
  libx11-dev libxext-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxfixes-dev libxrender-dev
  libxss-dev libxtst-dev libxkbcommon-dev
  libgl1-mesa-dev libegl1-mesa-dev libvulkan-dev vulkan-tools mesa-vulkan-drivers
  libasound2-dev libpulse-dev libudev-dev libdbus-1-dev
  libcurl4-openssl-dev libsecret-1-dev
  xvfb xauth openbox
)
missing=()
for p in "${packages[@]}"; do
  dpkg-query -W -f='${Status}' "$p" 2>/dev/null | grep -q "install ok installed" || missing+=("$p")
done
if [ ${#missing[@]} -gt 0 ]; then
  say "apt: installing ${missing[*]}"
  as=()
  [ "$(id -u)" = 0 ] || as=(sudo)
  # a hosted CI runner: the packages' triggers rebuild the boot image (15 s) and restart services, for a machine that
  # is thrown away after the job; neither is wanted there (a cloud container has no boot image to rebuild)
  if [ "${GITHUB_ACTIONS:-}" = true ] && [ -f /etc/initramfs-tools/update-initramfs.conf ]; then
    echo 'update_initramfs=no' | "${as[@]}" tee /etc/initramfs-tools/update-initramfs.conf > /dev/null || true
  fi
  # (another session may hold apt's lock: wait for it rather than fail; the environment through env, as sudo drops it)
  "${as[@]}" apt-get -o DPkg::Lock::Timeout=600 update -qq || echo "apt-get update reported errors: going on with the lists there are"
  "${as[@]}" env DEBIAN_FRONTEND=noninteractive NEEDRESTART_SUSPEND=1 \
    apt-get -o DPkg::Lock::Timeout=600 install -y -qq --no-install-recommends "${missing[@]}"
else
  say "apt: every package is installed"
fi

# ---- 2. CMake --------------------------------------------------------------------------------------------------------
cmake_ok() {
  command -v cmake >/dev/null || return 1
  local v; v=$(cmake --version | sed -nE '1s/^cmake version ([0-9]+)\.([0-9]+).*/\1 \2/p')
  set -- $v
  [ "${1:-0}" -gt 3 ] || { [ "${1:-0}" -eq 3 ] && [ "${2:-0}" -ge 30 ]; }
}
if cmake_ok; then
  say "cmake: $(cmake --version | head -1)"
else
  say "cmake: installing 4.1.3 with pip (FMinus and FunkGui need 3.30; this one is $(cmake --version 2>/dev/null | head -1 || echo none))"
  python3 -m pip install --quiet cmake==4.1.3 2>/dev/null || python3 -m pip install --quiet --break-system-packages cmake==4.1.3
  hash -r
  cmake_ok || { echo "bootstrap: pip installed cmake, and the cmake on the PATH is still $(command -v cmake): put pip's" \
                     "scripts directory ($(python3 -c 'import sysconfig; print(sysconfig.get_path("scripts"))')) first" >&2; exit 1; }
  cmake --version | head -1
fi

# ---- 3. FMinus's LLVM ------------------------------------------------------------------------------------------------
say "FMinus's prebuilt LLVM"
"$root/scripts/fetch-llvm.sh" linux-x64

# ---- 4. the sources --------------------------------------------------------------------------------------------------
say "the dependencies' sources, by git at their pins (cmake/TeePins.cmake)"
cmake -DTEE_DEPS_DIR="$TEE_DEPS_DIR" -P "$root/cmake/TeeFetch.cmake"

say "ready"
cat <<END
The dependency cache is $TEE_DEPS_DIR.
From $root:

  cmake --workflow --preset dev-verify          # configure, build with 3 jobs, run every test (build-dev/)

or a step at a time:

  cmake --preset dev && cmake --build --preset dev && ctest --preset dev
  build-dev/bin/tee-render --preset v8 --scenario free-revs --out out/v8-free-revs.wav
END
if [ "$TEE_DEPS_DIR" != /home/user/.tee-deps ]; then
  echo
  echo "TEE_DEPS_DIR is not the default: export TEE_DEPS_DIR=$TEE_DEPS_DIR before configuring."
fi
