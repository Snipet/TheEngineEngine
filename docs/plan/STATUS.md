# Status

*Kept by the lead. Times are UTC, 2026-11-24.*

| Package | State | Where it runs | Since | Notes |
|---|---|---|---|---|
| A: FunkGui desktop | running | local agent, `/home/user/FunkGui` | 11:22 | five commits pushed by 11:47 (GPU layer without JUCE, `DesktopHost`, the gallery tool, CI); its first CI run is building |
| K1, K2: the keel | running | local agent, `/home/user/work/keel` | 11:27 | pushed by 11:52: build, bootstrap, `sim/ref/`, `tee_host`, `tee-render`, CI; finishing the bench and tests |
| B: shell and application | running | local agent, `/home/user/work/shell` | 11:30 | interfaces pushed (`Sources.h`, `MainPanel.h`) |
| J1: interface kit | running | cloud session `session_01LFAGgRcCpuSF1ZriF9Koax` | 11:32 | first push 11:52: the panel code ported, 28 headless tests |
| K3: the split | running | local agent, `/home/user/work/split` | 11:55 | K3a first (the new contract over the unchanged reference) |
| C: host and tools | running | local agent, `/home/user/work/hosttools` | 11:55 | Schema lint and the scenario library first |
| I: vehicle | running | local agent, `/home/user/work/vehicle` | 12:00 | standalone first, integrated after K3b |
| F: forced induction | running | local agent, `/home/user/work/boost` | 12:00 | the same |
| E: valvetrain and induction | running | local agent, `/home/user/work/valvetrain` | 12:00 | the same |
| H: combustion and ECU | running | local agent, `/home/user/work/ecu` | 12:00 | the same |
| D, G | waiting for K3b | | | |
| J2 | waiting for J1's structure | | | |
| Wave 2 | after the gate (M2) | | | Q (performance) as soon as K3b lands |

## Landed

- 11:19 stage 1: the abstract plan, reviewed; the answers recorded.
- 11:45 stage 2: every file of this folder.

## Blocked, or wanting a decision

- Resolved 12:10: cloud sessions could not fetch FMinus's prebuilt LLVM (their GitHub access covers only this
  repository). The owner published the Linux archive as the pre-release `deps-llvm-23.1.3` here, and
  `scripts/fetch-llvm.sh` falls back to it. Packages started from now on can run in their own cloud sessions; the
  nine already running locally stay where they are.
