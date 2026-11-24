# Status

*Kept by the lead. Times are UTC, 2026-11-24.*

| Package | State | Where it runs | Since | Notes |
|---|---|---|---|---|
| A: FunkGui desktop | running | local agent, `/home/user/FunkGui` | 11:22 | stage 1 first: CI artefacts for Windows and macOS |
| K1, K2: the keel | running | local agent, `/home/user/work/keel` | 11:27 | first push when `tee-render` writes a WAV |
| B: shell and application | running | local agent, `/home/user/work/shell` | 11:30 | slice 1 is M1's second half |
| J1: interface kit | running | cloud session `session_01LFAGgRcCpuSF1ZriF9Koax` | 11:32 | first child session: also proves the mechanism |
| K3: the split | waiting for K2 | | | |
| C: host and tools | waiting for K2 | | | |
| I, F, E, H | waiting for K1 (the bootstrap) | | | standalone first, integrated after K3b |
| D, G | waiting for K3b | | | |
| J2 | waiting for J1's structure | | | |
| Wave 2 | after the gate (M2) | | | Q (performance) as soon as K3b lands |

## Landed

- 11:19 stage 1: the abstract plan, reviewed; the answers recorded.
- 11:45 stage 2: every file of this folder.

## Blocked, or wanting a decision

Nothing yet.
