#!/usr/bin/env bash
# scripts/fetch-llvm.sh: FMinus's prebuilt LLVM for this platform, into the dependency cache.
#
#   scripts/fetch-llvm.sh [linux-x64|macos-arm64|windows-x64]
#
# FMinus links LLVM statically, and building LLVM takes 10 to 30 minutes, so nothing here builds it. FMinus's Deps
# workflow publishes one archive a platform on the release deps-llvm-<version> of Snipet/FMinus, each beside its
# SHA-256: FMinus's deps cache's installs (LLVM, and Bison and Flex, or WinFlexBison on Windows). This script downloads
# the platform's archive, checks it against the release's .sha256, and unpacks it into $TEE_DEPS_DIR/install, where
# cmake/TeeDeps.cmake finds it. It does nothing when an LLVM of the pinned version is already there.
#
# The version and the repository are cmake/TeePins.cmake's (TEE_LLVM_VERSION, TEE_LLVM_RELEASE_REPOSITORY). The
# archive is fetched by its URL; where that is refused (a cloud container's proxy answers 403 for release archives),
# through `gh api` instead, which reaches the same asset.
#
# TEE_DEPS_DIR: the environment's; else /home/user/.tee-deps where that exists (a cloud container); else .tee-deps in
# this checkout, as cmake/TeeDeps.cmake has it.
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
pins="$root/cmake/TeePins.cmake"
version=$(sed -nE 's/^set\(TEE_LLVM_VERSION "([^"]+)"\).*/\1/p' "$pins")
repo=$(sed -nE 's/^set\(TEE_LLVM_RELEASE_REPOSITORY "([^"]+)"\).*/\1/p' "$pins")
[ -n "$version" ] && [ -n "$repo" ] || { echo "fetch-llvm: no TEE_LLVM_VERSION or TEE_LLVM_RELEASE_REPOSITORY in $pins" >&2; exit 1; }
tag="deps-llvm-$version"

platform=${1:-}
if [ -z "$platform" ]; then
  case "$(uname -s)-$(uname -m)" in
    Linux-x86_64) platform=linux-x64 ;;
    Darwin-arm64) platform=macos-arm64 ;;
    MINGW*-x86_64|MSYS*-x86_64|CYGWIN*-x86_64) platform=windows-x64 ;;
    *) echo "fetch-llvm: no prebuilt LLVM for $(uname -s) $(uname -m): there are linux-x64, macos-arm64 and" \
            "windows-x64; build FMinus once and give its .deps as TEE_DEPS_DIR instead" >&2; exit 1 ;;
  esac
fi

if [ -n "${TEE_DEPS_DIR:-}" ]; then deps=$TEE_DEPS_DIR
elif [ -d /home/user/.tee-deps ]; then deps=/home/user/.tee-deps
else deps="$root/.tee-deps"; fi
mkdir -p "$deps/install"
deps=$(cd "$deps" && pwd)

for config in "$deps"/install/llvm-"$version"-*/lib/cmake/llvm/LLVMConfig.cmake; do
  if [ -f "$config" ]; then
    echo "fetch-llvm: LLVM $version is in ${config%/lib/cmake/llvm/LLVMConfig.cmake}: nothing to do"
    exit 0
  fi
done

name="fminus-deps-llvm-$version-$platform.tar.gz"
url="https://github.com/$repo/releases/download/$tag/$name"
work="$deps/install/.fetch-llvm.$$"
rm -rf "$work"
mkdir -p "$work/unpacked"
trap 'rm -rf "$work"' EXIT
cd "$work"   # relative names below: Git Bash's tar takes D:\... for a remote host

echo "fetch-llvm: $name from the release $tag of $repo"
if curl -fsSL --retry 3 -o "$name" "$url" 2>/dev/null && curl -fsSL --retry 3 -o "$name.sha256" "$url.sha256" 2>/dev/null; then
  echo "fetch-llvm: downloaded by URL"
elif command -v gh >/dev/null; then
  echo "fetch-llvm: the URL is refused here; asking the GitHub API for the same assets (gh api)"
  for asset in "$name" "$name.sha256"; do
    id=$(gh api "repos/$repo/releases/tags/$tag" --jq ".assets[] | select(.name == \"$asset\") | .id")
    [ -n "$id" ] || { echo "fetch-llvm: the release $tag of $repo has no asset $asset" >&2; exit 1; }
    gh api -H "Accept: application/octet-stream" "repos/$repo/releases/assets/$id" > "$asset"
  done
else
  echo "fetch-llvm: could not download $url, and there is no gh to ask the GitHub API with" >&2
  exit 1
fi

if command -v sha256sum >/dev/null; then sha256sum -c "$name.sha256"; else shasum -a 256 -c "$name.sha256"; fi
tar -xzf "$name" -C unpacked
# Each install into place, unless another run put it there meanwhile.
for d in unpacked/*/; do
  d=${d%/}
  base=${d#unpacked/}
  if [ -e "$deps/install/$base" ]; then
    echo "fetch-llvm: $deps/install/$base is there already: kept"
  else
    mv "$d" "$deps/install/$base"
    echo "fetch-llvm: $deps/install/$base"
  fi
done
ls -d "$deps"/install/llvm-"$version"-*/lib/cmake/llvm/LLVMConfig.cmake >/dev/null \
  || { echo "fetch-llvm: the archive held no LLVM $version" >&2; exit 1; }
