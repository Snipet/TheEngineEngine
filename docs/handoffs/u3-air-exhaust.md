# U3: AIR and EXHAUST (handoff)

Package U3, branch `claude/awesome-gates-ntdb58`. Pages: `docs/ui/air.md` and `docs/ui/exhaust.md`. Paths: `ui/views/{air,exhaust}/`, `tests/ui/views/{Air,Exhaust}Tests.cpp`.

**Done** (`tee-view-tests`, 20 tests, all pass on Linux):
- `ui.exhaust.*`: model, stage, builds, select, absent, rest, slow, sizes, inspector, sheet, framestage.
- `ui.air.*`: model, stage, builds, select, absent, rest, sizes, inspector, framestage (at U1's real stage for 1100x700 and 1600x1000: every part inside, none overlapping).
- Builds: EXHAUST the card's four, a V12 on logs, a turbo boxer; AIR an ITB four, a turbo with anti-lag and nitrous, twin, Roots, sequential, compound, a centrifugal blower.

**Goldens**: 16 `air.*` and 15 `exhaust.*`, at density 1 and 2, at 1100x700 and 1600x1000, in graphite and paper.
- All PNGs from the software rasteriser (no GPU here). Looked at before blessing: every build, select, absent, paper, U1's frame.air and frame.exhaust.
- Contact sheet: `out/ui/sheet.u3.png` (test `ui.exhaust.sheet`).
**Read today, on the branch**:
- Telemetry: `engine.manifold`, `engine.throttle`, `boost.pressure`, `exhaust.portTemperature`.
- Scope: `pipePressure`.
- Fields: `engine.layout`, `boost.kind`, `exhaust.headers`, `exhaust.pipe`, `env.*`.

**Read but not on the branch yet** (each draws a dash or a neutral tint; one table each, `ExhaustPaths`/`ExhaustNames`, `AirPaths`/`AirNames`):
- Fields: buildspec.fm's XSpec as `exhaust.<field>[i]`; machinespec.fm's as `boost.<field>`; lab-spec.fm's as `intake.<field>`; `boost.layout`; `engine.displacement`.
- Exhaust telemetry:
  - temperatures: `exhaust.collectorTemperature`, `exhaust.tailTemperature`;
  - walls: `exhaust.wallPrimaries|Collectors|Merges|Main|Link|Mufflers|Tails`;
  - `exhaust.tailLevel[s]`, `exhaust.valve[s]`.
- Boost telemetry:
  - turbos: `boost.shaftSpeed[i]`, `boost.ratio[i]`, `boost.flow[i]` (corrected), `boost.compressorTemperature[i]`, `boost.gate[0]`, `boost.vanes[0]`;
  - charge and supercharger: `boost.target`, `boost.intercoolerTemperature`, `boost.superchargerPower`, `boost.bypass`, `boost.blowOff`;
  - extras: `boost.antiLag`, `boost.nitrous`, `boost.bottle`.
- Intake telemetry: `intake.boxPressure`, `intake.plenumTemperature`.

**Clashes to settle at integration**: XSpec's `crossover` and `muffler` collide with spec.exhaust's dials of those names. The model reads a 3-way choice, the demo's switch, or the old dial. The arrays are read as `exhaust.muffler[s]`.

**Demo data**: in my directories (`ExhaustDemo.cpp`, `AirDemo.cpp`), one function each, for U1's hook.
**Wanted**:
- From U1:
  - `frame.ribbon` pictures the AIR stage, so its golden moves with my work. Please re-bless it, or picture a placeholder there.
  - Today `ui.frame.workspaces` fails on scope.gui, and `ui.build.frame` fails on engine.bankAngle fields: not mine.
- From the lead: a simulator "slow time" control would let SLOW show the waves truly slowed. Today it replays the
  copies taken at frame rate at an eighth of their pace.

**Not run**: macOS and Windows builds (CI); the GPU path. **Kit**: no change. FunkGui and FMinus: no issues met.
