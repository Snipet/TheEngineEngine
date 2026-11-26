# U3: AIR and EXHAUST (handoff)

Package U3, branch `claude/awesome-gates-ntdb58`. Pages: `docs/ui/air.md` and `docs/ui/exhaust.md`. Paths: `ui/views/{air,exhaust}/`, `tests/ui/views/{Air,Exhaust}Tests.cpp`.

**Done** (`tee-view-tests`, 30 U3 tests, all pass on Linux):
- `ui.exhaust.*`: model, stage, builds, select, absent, rest, slow, sizes, inspector, framestage, text, effects, twostroke, ductmid, cost; sheet.pictures (nightly).
- `ui.air.*`: model, stage, builds, select, absent, rest, sizes, inspector, framestage, text, textcheck, scopemap, ram, mismatch.
- Text checks: no two texts meet on any build at both frame stages. On the two-stroke, no trace crosses a label either
  (the port plot's OPENS and CLOSES now sit above its frame).
- Builds: EXHAUST four-stroke (card's four, V12 on logs, turbo boxer) and two-stroke (single, twin with silencers).
  AIR: ITB four, turbo with anti-lag and nitrous, twin, Roots, sequential, compound, centrifugal, variable runners.

**Goldens**: `air.*` and `exhaust.*` at density 1 and 2, 1100x700 and 1600x1000, graphite and paper.
- All from the software rasteriser (no GPU here); every PNG looked at before blessing.
- Contact sheet: `out/ui/sheet.u3.png` (`ui.exhaust.sheet.pictures`: nightly, a picture only).

**Bound to the simulator's names**:
- EXHAUST, card G: crossKind, mufKind; tel.exhaust gas/wallTemperature[7], ends/endLevel/endSlot, valve, flame.
  Scope: exhaustPressure/exhaustX (cylinder 1's path), ductUsed/ductNear/ductFar (every pipe's own pulse).
- AIR, card F: tel.boost.*. The Scope's compressor map is used when present; otherwise the mirror of compressor.fm.
- AIR, card E: intake names, and ram tuning from the runner length (mirrors E's IntegratedTest ring()).
- Two-stroke, card T: engine.ts* fields; tel.engine.tsPipeTemperature (else 680 K, said); Scope portExhaust when
  `degrees` is 360, pipePressure's six along the chamber at chamber.fm's section middles ([6], the crankcase, not drawn).

**Wanted from U1**: the demo's Scope is a four-stroke's (720 degrees) for a two-stroke build too, so the port plot
says "no port pressure" there. `exhaust::exhaustTwoStrokePort(b, out, max)` gives card T's measured turn to serve.

**Checked live**: card G's wiring (cf267fb, 95d7d16) under the real simulator, exhaust.model 1, default V8: every
tel.exhaust and Scope name above reads; ductUsed's slots (0-7, 32-33, 48, 53-55, 59, 63, 68-70, 74, 78) are slots.fm's.
G's `ductMid` (dacb9f8) bound: each pipe's trace bends through its middle (`ui.exhaust.ductmid`).
Card T's two-stroke (two-stroke-mx125.json's spec, custom engine): degrees 360, pipePoints 7, portExhaust 360
entries (absolute), tsPipeTemperature 614 K at 12 500 rpm: each read as the stage expects.

**Whole suite**: `tee-view-tests all` 129 of 129 pass.

**Wanted from the lead**: a simulator "slow time" control would let SLOW show the waves truly slowed. Today SLOW replays
the frame-rate copies at an eighth of their pace.

**Not run**: macOS and Windows builds (CI); the GPU path. **Kit**: no change. FunkGui and FMinus: no issues met.

**Added by U1 for U3, 26 Nov 2026** (on the lead's word; U3's session idle since 24 Nov): `warnings()` in
`ui/views/air/AirWorkspace.cpp` now words two more of card N2's `boost.mismatch` bits, `Kind::turbo` like the others:
16, the turbines' nozzle far from the build's design point's (`boost.turbineHas` and `turbineWants`, an area; the bit
existed and printed nothing); and 128, a turbo the build's exhaust leaves with none of the engine's flow (N2's new
flag, 26 Nov: one click on the Aspiration dial could leave one at rest; the cure is the next window's). The demo holds
the two new names at 0 (`AirDemo.cpp`). `air::airWarnings(w)` gives the lines' words for tests. Test:
`air.mismatch.turbo`, which checks each bit's line present when the bit is set and gone when it's clear, and both
together clear of each other. No golden moved.

**Added by U1 for U3, 26 Nov 2026, temporary** (the lead's finding: the engine never reads `env.pressure`; only the
car's drag does; sim/engine.fm's literal 101325): set away from 101325 by more than 500 Pa, `warnings()` adds "AIR
PRESSURE moves the car's drag only: this engine breathes sea-level air (101.3 kPa)" (Kind::box), and `resolveAir` holds
`AirBuild::ambientP` at 101325 (the setting kept as `docAmbientP`), so the absolutes the row prints and the vacuum are
the engine's as it is. BOTH LEAVE with the next window's item 15 (the engine reads env.pressure): then the line goes and
`ambientP = docAmbientP`. Probe `air::airAbsolutes`. Test `air.pressure` (the line absent at 101325 and ±400, present at
70 and 105 kPa; the absolutes at 70 kPa equal to sea level's); picture out/ui/air.pressure.line.png.
