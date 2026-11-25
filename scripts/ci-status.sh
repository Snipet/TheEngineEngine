#!/usr/bin/env bash
# scripts/ci-status.sh [branch]: what CI last said, for each platform: the newest run of the Build workflow in which
# that platform's job finished, whether it passed, and if not the step it failed in and the first errors of its log.
#
#   scripts/ci-status.sh                       # the branch checked out (the integration branch, normally)
#   scripts/ci-status.sh main
#
# Why "the newest run in which the job finished": each platform's job queues behind that platform's job in flight, and
# one still waiting is replaced by a newer push, so with many people pushing most jobs are "cancelled" without having
# built anything, and a run can be "cancelled" for one platform and finished for another. Cancelled and skipped jobs
# say nothing and are skipped; a job in flight is never cancelled by a push.
#
# Linux is the only system an agent in a cloud container can build on: macOS (Apple's clang, arm64) and Windows
# (clang-cl) are built here and nowhere else, so look at them after you push, not only at Linux.
#
# It needs gh, signed in (GH_TOKEN or `gh auth login`); it only reads. TEE_CI_REPO names another repository.
set -euo pipefail

repo=${TEE_CI_REPO:-Snipet/TheEngineEngine}
branch=${1:-$(git rev-parse --abbrev-ref HEAD 2>/dev/null || true)}
case "$branch" in ''|HEAD) branch=claude/awesome-gates-ntdb58 ;; esac   # a detached checkout: the integration branch
platforms=(linux-x64 macos-arm64 windows-x64)                     # the fast tier: the default tests
soaks=(linux-x64-soak macos-arm64-soak windows-x64-soak)           # the soak tier: [nightly], only when asked
required=("${platforms[@]}")                                        # what the search waits for (a soak is rare)
command -v gh >/dev/null || { echo "ci-status: no gh on the PATH" >&2; exit 1; }

# The Build runs of the branch, newest first, a page of 100 at a time until every platform and tier has a finished
# job (a run cancelled as a whole may still hold one platform's finished job, and with twenty sessions pushing the
# branch takes more pushes than that while one job runs). Each page's jobs are fetched eight at once.
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fetch_jobs() {
  gh api "repos/$repo/actions/runs/$1/jobs" \
    --jq '.jobs[] | "\(.name) \(.id) \(.status) \(.conclusion // "-") \(.steps | length) "
          + ([.steps[]? | select(.conclusion == "failure") | .name] | join("; "))' > "$tmp/$1" 2>/dev/null || true
}
export -f fetch_jobs; export repo tmp

echo "CI on $branch of $repo (https://github.com/$repo/actions)"
declare -A found=() report=() noted=()
done_yet() { for p in "${required[@]}"; do [ -n "${found[$p]:-}" ] || return 1; done; }
page=1; seen=0
while ! done_yet && [ "$page" -le "${TEE_CI_PAGES:-10}" ]; do
runs=$(gh api "repos/$repo/actions/runs?branch=$branch&per_page=100&page=$page" \
  --jq '.workflow_runs[] | select(.name == "Build")
        | "\(.id) \(.status) \(.head_sha[0:7]) \(.created_at)"')
[ -n "$runs" ] || break
awk '{print $1}' <<< "$runs" | xargs -P8 -n1 bash -c 'fetch_jobs "$1"' _
while read -r run status sha created; do
  ! done_yet || break
  seen=$((seen + 1))
  jobs=$(cat "$tmp/$run" 2>/dev/null || true)
  if [ "$status" = in_progress ]; then
    echo
    echo "in flight: run $run at $sha, pushed $created"
    echo "$jobs" | while read -r name _ jobStatus conclusion _; do
      case " ${platforms[*]} ${soaks[*]} " in *" $name "*) echo "  $name: $jobStatus $([ "$conclusion" = - ] || echo "($conclusion)")" ;; esac
    done
  elif [ "$status" != completed ]; then
    echo
    echo "waiting for the run in flight: run $run at $sha, pushed $created (a newer push will replace it)"
  fi
  for platform in "${platforms[@]}" "${soaks[@]}"; do
    [ -z "${found[$platform]:-}" ] || continue
    line=$(echo "$jobs" | grep "^$platform " || true)
    [ -n "$line" ] || continue
    read -r _ job jobStatus conclusion steps step <<< "$line"
    [ "$jobStatus" = completed ] || continue
    # a job cancelled before any step ran because no hosted runner took it (macOS arm64 at busy hours) is no result: it
    # is named once, so that nobody reads it as one, and the search goes on for the platform's newest finished job
    if [ "$conclusion" = cancelled ] && [ "$steps" = 0 ] && [ -z "${noted[$platform]:-}" ] &&
       gh api "repos/$repo/check-runs/$job/annotations" --jq '.[].message' 2>/dev/null | grep -q 'not acquired'; then
      noted[$platform]=1
      echo "  ($platform: not acquired (runner capacity) at $sha, run $run: no runner took the job; not a result)"
    fi
    case "$conclusion" in success|failure) ;; *) continue ;; esac   # a skipped or cancelled job says nothing
    found[$platform]=1
    text="$platform: $conclusion, at $sha ($created), run $run"$'\n'"  https://github.com/$repo/actions/runs/$run/job/$job"
    if [ "$conclusion" = failure ]; then
      text+=$'\n'"  failed in: ${step:-a step the API does not name}"
      # the log's first errors: a compiler's, a linker's, ninja's, CMake's, ctest's (not the workflow's own script,
      # which the log echoes in colour)
      text+=$'\n'$(gh api "repos/$repo/actions/jobs/$job/logs" 2>/dev/null \
        | grep -E "error:|error [A-Z]+[0-9]+:|FAILED:|\*\*\*Failed|\*\*\*Timeout|\*\*\*Not Run|\*\*\*Exception|Undefined symbols|undefined symbol|CMake Error|##\[error\]" \
        | grep -v $'\x1b\\[36;1m' | sed -E 's/^[0-9T:.-]+Z //' | cut -c1-240 | head -12 | sed 's/^/    /' || true)
    fi
    report[$platform]=$text
  done
done <<< "$runs"
page=$((page + 1))
done

echo
echo "== the fast tier (the default tests, every push)"
for platform in "${platforms[@]}"; do
  echo
  echo "${report[$platform]:-$platform: no finished job among the last $seen runs}"
done
echo
echo "== the soak tier ([nightly] tests, when asked: [soak] Linux, [soak-all] all three, by hand, each night)"
for platform in "${soaks[@]}"; do
  if [ -n "${report[$platform]:-}" ]; then echo; echo "${report[$platform]}"
  elif [ "$platform" = linux-x64-soak ]; then echo; echo "$platform: no finished soak among the last $seen runs"
  fi
done

# main: the lead fast-forwards it to the integration branch's tip at each checkpoint (01-rules.md section 2); one line
# a platform, its newest finished job, so a check-in can say "main is at X, green on a, b, c"
if [ "$branch" != main ]; then
  echo
  main_sha=$(gh api "repos/$repo/branches/main" --jq '.commit.sha[0:7]' 2>/dev/null || echo "?")
  echo "== main (at $main_sha; the lead moves it forward at each checkpoint)"
  mruns=$(gh api "repos/$repo/actions/runs?branch=main&per_page=30" \
    --jq '.workflow_runs[] | select(.name == "Build") | "\(.id) \(.status) \(.head_sha[0:7]) \(.created_at)"' 2>/dev/null || true)
  if [ -z "$mruns" ]; then echo "  no Build run on main yet"; else
    awk '{print $1}' <<< "$mruns" | xargs -P8 -n1 bash -c 'fetch_jobs "$1"' _
    for platform in "${platforms[@]}"; do
      line=""
      while read -r run status sha created; do
        l=$(grep "^$platform " "$tmp/$run" 2>/dev/null || true)
        read -r _ _ s c _ <<< "${l:-x x x x}"
        if [ "$s" = completed ] && { [ "$c" = success ] || [ "$c" = failure ]; }; then line="$c at $sha ($created), run $run"; break; fi
        [ "$s" = in_progress ] && [ -z "$line" ] && line="in flight at $sha, run $run (no finished job before it among the last 30 runs)"
      done <<< "$mruns"
      echo "  $platform: ${line:-no finished job among the last 30 runs on main}"
    done
  fi
fi
