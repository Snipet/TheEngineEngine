#!/bin/sh
# Builds qbench (docs/measurements/tools/qbench.cpp) against a configured and built build-dev tree, with the
# environment-overridable copy of host/src/Program.cpp (patch-program.sh) linked ahead of libtee_host.a.
#   docs/measurements/tools/build.sh [out-dir]       (default: build-dev/q)
set -e
root=$(cd "$(dirname "$0")/../../.." && pwd)
bd="$root/build-dev"; out="${1:-$bd/q}"; mkdir -p "$out"
"$root/docs/measurements/tools/patch-program.sh" "$root" "$out/Program.cpp"
inc="-I$root/host/include -isystem ${TEE_DEPS_DIR:-/home/user/.tee-deps}/src/fminus-0.1.0-5b665d7/include -isystem $bd/_deps/fminus/include"
for f in "$out/Program.cpp" "$root/docs/measurements/tools/qbench.cpp"; do
  clang++ -O2 -std=c++20 -ffp-contract=off -DTEE_SOURCE_DIR=\"$root\" $inc -c "$f" -o "$out/$(basename "$f" .cpp).o"
done
# the link line of tee-bench, its own objects replaced by ours
link=$(cd "$bd" && ninja -t commands bin/tee-bench | tail -1 | sed 's/^: && //; s/ && :$//')
objs=$(echo "$link" | grep -o 'tools/CMakeFiles/tee-bench.dir/[^ ]*\.o' | tr '\n' ' ')
link=$(echo "$link" | sed "s#-o bin/tee-bench#-o $out/qbench#; s#-Xlinker --dependency-file=[^ ]*##")
for o in $objs; do link=$(echo "$link" | sed "s#$o##"); done
(cd "$bd" && eval "$(echo "$link" | sed "s#-o $out/qbench#$out/Program.o $out/qbench.o -o $out/qbench#")")
echo "$out/qbench"
