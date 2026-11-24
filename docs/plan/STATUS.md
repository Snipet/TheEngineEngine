# Status

*Kept by the lead. Times are UTC, 2026-11-24.*

| Package | State | Where it runs | Since | Notes |
|---|---|---|---|---|
| A: FunkGui desktop | running | local agent, `/home/user/FunkGui` | 11:22 | five commits pushed by 11:47 (GPU layer without JUCE, `DesktopHost`, the gallery tool, CI); its first CI run is building |
| K1, K2: the keel | running | local agent, `/home/user/work/keel` | 11:27 | pushed by 11:52: build, bootstrap, `sim/ref/`, `tee_host`, `tee-render`, CI; finishing the bench and tests |
| B: shell and application | running | local agent, `/home/user/work/shell` | 11:30 | interfaces pushed (`Sources.h`, `MainPanel.h`) |
| J1: interface kit | **landed 12:04** | cloud session | 11:32 | the panel code ported (36 files, provenance in `ui/kit/SEED.tsv`), `RefPanel`, shaders compiled for Metal and SPIR-V; 29 headless tests; no shader drawn yet (no GPU path ran) |
| K3: the split | running | local agent, `/home/user/work/split` | 11:55 | K3a first (the new contract over the unchanged reference) |
| C: host and tools | running | local agent, `/home/user/work/hosttools` | 11:55 | Schema lint and the scenario library first |
| I: vehicle | running | local agent, `/home/user/work/vehicle` | 12:00 | standalone first, integrated after K3b |
| F: forced induction | running | local agent, `/home/user/work/boost` | 12:00 | the same |
| E: valvetrain and induction | running | local agent, `/home/user/work/valvetrain` | 12:00 | the same |
| H: combustion and ECU | running | local agent, `/home/user/work/ecu` | 12:00 | the same |
| D: mechanics and layouts | running | cloud session `session_01LFAGgTFpHiA9YGryQgFytL` | 12:07 | standalone first (layouts from geometry, inertia, shaking, friction, work); the integrator's job after K3b |
| G: exhaust builder | running | cloud session `session_01LFAGgSkq1paVPmFHkoNNei` | 12:08 | standalone first (the network on the reference's `Duct`); integrated after K3b |
| J2: widgets, binding, the frame | running | cloud session `session_01LFAGgQnadEqmEkF3xi2V1S` | 12:05 | widgets and units first; the binding when card C's Schema lands |
| Wave 2 | after the gate (M2) | | | Q (performance) as soon as K3b lands |

## Landed

- 11:19 stage 1: the abstract plan, reviewed; the answers recorded.
- 11:45 stage 2: every file of this folder.
- 11:52 the keel's first slice: the build, the bootstrap, `sim/ref/`, `tee_host`, `tee-render`, CI (first run green).
- 12:04 J1: the interface kit's first slice (`docs/handoffs/j1-ui-kit.md`).

## Blocked, or wanting a decision

- Resolved 12:10: cloud sessions could not fetch FMinus's prebuilt LLVM (their GitHub access covers only this
  repository). The owner published the Linux archive as the pre-release `deps-llvm-23.1.3` here, and
  `scripts/fetch-llvm.sh` falls back to it. A cloud session then bootstrapped in 25 s and built in 3.5 min
  (12:06). It fetched the archive by plain URL from FMinus's own release: release assets of a public repository
  are reachable after all (only the API and source archives are refused), so the mirror is a fallback that has
  not yet been needed. Packages started from now on run in their own cloud sessions; the nine already running
  locally stay where they are.
