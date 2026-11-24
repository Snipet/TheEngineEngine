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
soaks=(linux-x64-soak macos-arm64-soak windows-x64-soak)           # the soak tier: [nightly] (macOS, Windows on [soak])
required=("${platforms[@]}" linux-x64-soak)                         # what the search waits for
command -v gh >/dev/null || { echo "ci-status: no gh on the PATH" >&2; exit 1; }

# The Build runs of the branch, newest first (a run cancelled as a whole may still hold one platform's finished job).
runs=$(gh api "repos/$repo/actions/runs?branch=$branch&per_page=60" \
  --jq '.workflow_runs[] | select(.name == "Build")
        | "\(.id) \(.status) \(.head_sha[0:7]) \(.created_at)"')
[ -n "$runs" ] || { echo "ci-status: no Build run on $branch of $repo"; exit 0; }

echo "CI on $branch of $repo (https://github.com/$repo/actions)"
declare -A found=() report=()
done_yet() { for p in "${required[@]}"; do [ -n "${found[$p]:-}" ] || return 1; done; }
while read -r run status sha created; do
  ! done_yet || break
  jobs=$(gh api "repos/$repo/actions/runs/$run/jobs" \
    --jq '.jobs[] | "\(.name) \(.id) \(.status) \(.conclusion // "-") "
          + ([.steps[]? | select(.conclusion == "failure") | .name] | join("; "))')
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
    read -r _ job jobStatus conclusion step <<< "$line"
    [ "$jobStatus" = completed ] || continue
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

echo
echo "== the fast tier (the default tests, every push)"
for platform in "${platforms[@]}"; do
  echo
  echo "${report[$platform]:-$platform: no run among the last 60 in which its job finished}"
done
echo
echo "== the soak tier ([nightly] tests: Linux every push; macOS and Windows when a commit says [soak])"
for platform in "${soaks[@]}"; do
  if [ -n "${report[$platform]:-}" ]; then echo; echo "${report[$platform]}"
  elif [ "$platform" = linux-x64-soak ]; then echo; echo "$platform: no run among the last 60 in which its job finished"
  fi
done
