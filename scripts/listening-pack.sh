#!/usr/bin/env bash
# scripts/listening-pack.sh: checkpoint M2's listening pack (docs/checkpoints/M2.md). Renders, with tee-render, every
# line of scripts/listening-pack.tsv into a folder of short WAV files, each group loudness-matched by tee-fuzz --match,
# named so that a group's files sort side by side. Nothing it writes is committed.
#
#   scripts/listening-pack.sh [--quick] [--bin DIR] [--out DIR] [--level DBFS]
#
#   --quick      only the ten most telling files (the TSV's quick column)
#   --bin DIR    where tee-render and tee-fuzz are: a build's bin/ (build-release/bin, build-dev/bin) or an unpacked CI
#                artefact (tee-macos-arm64/). Default: the first of build-release/bin, build-dev/bin that has both.
#   --out DIR    the folder (out/listening-pack, or out/listening-pack-quick)
#   --level D    each group's gated RMS in dBFS (-20)
#
# Run it from a checkout of the branch (the scenarios are in content/scenarios/combo/); the simulator and the builds
# are the checkout's sim/ and content/. Takes about a minute a file on a laptop core (the renders run one at a time).
set -u
cd "$(dirname "$0")/.."
root="$(pwd)"

quick=0
bin=""
out=""
level=-20
while [ $# -gt 0 ]; do
  case "$1" in
    --quick) quick=1 ;;
    --bin) bin="$2"; shift ;;
    --out) out="$2"; shift ;;
    --level) level="$2"; shift ;;
    -h|--help) sed -n '2,17p' "$0"; exit 0 ;;
    *) echo "listening-pack: unknown option $1 (--help)" >&2; exit 2 ;;
  esac
  shift
done

exe() { if [ -x "$1/$2" ]; then echo "$1/$2"; elif [ -x "$1/$2.exe" ]; then echo "$1/$2.exe"; fi; }
if [ -z "$bin" ]; then
  for d in build-release/bin build-dev/bin; do
    if [ -n "$(exe "$d" tee-render)" ] && [ -n "$(exe "$d" tee-fuzz)" ]; then bin="$d"; break; fi
  done
fi
render="$(exe "${bin:-.}" tee-render)"
fuzz="$(exe "${bin:-.}" tee-fuzz)"
if [ -z "$render" ]; then
  echo "listening-pack: no tee-render (give --bin: a build's bin/ or an unpacked CI artefact)" >&2
  exit 2
fi
if [ -z "$out" ]; then
  if [ "$quick" = 1 ]; then out=out/listening-pack-quick; else out=out/listening-pack; fi
fi
mkdir -p "$out"
readme="$out/README.txt"
{
  echo "TheEngineEngine listening pack (checkpoint M2): $(date -u '+%Y-%m-%d %H:%M UTC')"
  echo "commit: $(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
  echo "tee-render: $render"
  echo "What to listen for in each group, and the yes-or-no lines: docs/checkpoints/M2.md"
  echo "Each group is matched to $level dBFS gated RMS (tee-fuzz --match); compare within a group, not across."
  echo
} > "$readme"

rendered=0
skipped=0
failed=0
group=""
files=()
flush() {
  if [ ${#files[@]} -gt 0 ]; then
    if [ "${group%-raw}" != "$group" ]; then
      echo "  $group: not loudness-matched (its question is the loudness)" >> "$readme"
    elif [ -n "$fuzz" ]; then
      "$fuzz" --match "$level" "${files[@]}" >> "$readme" 2>&1 || echo "  (the level match failed)" >> "$readme"
    else
      echo "  (no tee-fuzz beside tee-render: this group is not loudness-matched)" >> "$readme"
    fi
  fi
  files=()
}

while IFS=$'\t' read -r g label q build scenario define; do
  case "$g" in ''|'#'*) continue ;; esac
  [ "$quick" = 1 ] && [ "$q" != 1 ] && continue
  if [ "$g" != "$group" ]; then flush; group="$g"; fi
  wav="$out/$g-$label.wav"
  if [ ! -f "content/builds/$build" ]; then
    echo "skipped $g-$label: content/builds/$build is not there yet (its package has not landed)" | tee -a "$readme"
    skipped=$((skipped + 1))
    continue
  fi
  echo "rendering $g-$label ($build, $(basename "$scenario"))"
  log="$out/.$g-$label.log"
  defines=()
  [ -n "${define:-}" ] && defines=(--define "$define")
  if "$render" --build "content/builds/$build" --scenario "$scenario" --out "$wav" --csv - \
      --sim "$root/sim" --content "$root/content" ${defines[@]+"${defines[@]}"} > "$log" 2>&1; then
    files+=("$wav")
    rendered=$((rendered + 1))
    # a build field this program does not have yet: the pair may sound the same
    if grep -qi "unknown\|kept unread\|not in the program" "$log"; then
      echo "note $g-$label: the build names fields this program lacks (their package has not landed): $(grep -i -m1 "unknown\|kept unread\|not in the program" "$log")" >> "$readme"
    fi
  else
    echo "FAILED $g-$label: tee-render exited $? (its words: $log)" | tee -a "$readme"
    failed=$((failed + 1))
  fi
done < scripts/listening-pack.tsv
flush

echo >> "$readme"
echo "$rendered rendered, $skipped skipped, $failed failed" | tee -a "$readme"
echo "the pack: $out"
[ "$failed" = 0 ]
