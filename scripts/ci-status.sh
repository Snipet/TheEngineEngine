#!/usr/bin/env bash
# scripts/ci-status.sh [branch]: what CI last said, for each platform: the newest run of the Build workflow in which
# that platform's job finished, whether it passed, and if not the step it failed in and the first errors of its log.
#
#   scripts/ci-status.sh                       # the branch checked out (the integration branch, normally)
#   scripts/ci-status.sh main
#
# Why "the newest run in which the job finished": pushes to one branch queue behind the run in flight, and a run
# still waiting is replaced by a newer push, so with many people pushing most runs are "cancelled" without having
# built anything. Those say nothing and are skipped. A run in flight is never cancelled by a push; it is listed first.
#
# Linux is the only system an agent in a cloud container can build on: macOS (Apple's clang, arm64) and Windows
# (clang-cl) are built here and nowhere else, so look at them after you push, not only at Linux.
#
# It needs gh, signed in (GH_TOKEN or `gh auth login`); it only reads. TEE_CI_REPO names another repository.
set -euo pipefail

repo=${TEE_CI_REPO:-Snipet/TheEngineEngine}
branch=${1:-$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo claude/awesome-gates-ntdb58)}
platforms=(linux-x64 macos-arm64 windows-x64)
command -v gh >/dev/null || { echo "ci-status: no gh on the PATH" >&2; exit 1; }

# The Build runs of the branch that were not replaced while they waited, newest first.
runs=$(gh api "repos/$repo/actions/runs?branch=$branch&per_page=60" \
  --jq '.workflow_runs[] | select(.name == "Build" and .conclusion != "cancelled")
        | "\(.id) \(.status) \(.head_sha[0:7]) \(.created_at)"')
[ -n "$runs" ] || { echo "ci-status: no Build run on $branch of $repo that was not cancelled while it waited"; exit 0; }

echo "CI on $branch of $repo (https://github.com/$repo/actions)"
declare -A found=()
while read -r run status sha created; do
  [ "${#found[@]}" -lt "${#platforms[@]}" ] || break
  jobs=$(gh api "repos/$repo/actions/runs/$run/jobs" \
    --jq '.jobs[] | "\(.name) \(.id) \(.status) \(.conclusion // "-") "
          + ([.steps[]? | select(.conclusion == "failure") | .name] | join("; "))')
  if [ "$status" = in_progress ]; then
    echo
    echo "in flight: run $run at $sha, pushed $created"
    echo "$jobs" | while read -r name _ jobStatus conclusion _; do
      case " ${platforms[*]} " in *" $name "*) echo "  $name: $jobStatus $([ "$conclusion" = - ] || echo "($conclusion)")" ;; esac
    done
  elif [ "$status" != completed ]; then
    echo
    echo "waiting for the run in flight: run $run at $sha, pushed $created (a newer push will replace it)"
  fi
  for platform in "${platforms[@]}"; do
    [ -z "${found[$platform]:-}" ] || continue
    line=$(echo "$jobs" | grep "^$platform " || true)
    [ -n "$line" ] || continue
    read -r _ job jobStatus conclusion step <<< "$line"
    [ "$jobStatus" = completed ] || continue
    case "$conclusion" in success|failure) ;; *) continue ;; esac   # a skipped or cancelled job says nothing
    found[$platform]=1
    echo
    echo "$platform: $conclusion, at $sha ($created), run $run"
    echo "  https://github.com/$repo/actions/runs/$run/job/$job"
    if [ "$conclusion" = failure ]; then
      echo "  failed in: ${step:-a step the API does not name}"
      # the log's first errors: a compiler's, a linker's, ninja's, CMake's, ctest's (not the workflow's own script,
      # which the log echoes in colour)
      gh api "repos/$repo/actions/jobs/$job/logs" 2>/dev/null \
        | grep -E "error:|error [A-Z]+[0-9]+:|FAILED:|\*\*\*Failed|\*\*\*Timeout|\*\*\*Not Run|\*\*\*Exception|Undefined symbols|undefined symbol|CMake Error|##\[error\]" \
        | grep -v $'\x1b\\[36;1m' | sed -E 's/^[0-9T:.-]+Z //' | cut -c1-240 | head -12 | sed 's/^/    /' || true
    fi
  done
done <<< "$runs"

for platform in "${platforms[@]}"; do
  [ -n "${found[$platform]:-}" ] || { echo; echo "$platform: no run among the last 60 in which its job finished"; }
done
