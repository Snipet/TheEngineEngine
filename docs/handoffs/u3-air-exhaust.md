# U3: AIR and EXHAUST (handoff)

Package U3, branch `claude/awesome-gates-ntdb58`. Pages: `docs/ui/air.md` and `docs/ui/exhaust.md`. Paths: `ui/views/{air,exhaust}/`, `tests/ui/views/{Air,Exhaust}Tests.cpp`.

**Done** (`tee-view-tests`, 28 U3 tests, all pass on Linux):
- `ui.exhaust.*`: model, stage, builds, select, absent, rest, slow, sizes, inspector, framestage, text, effects, twostroke, ductmid; sheet.pictures (nightly).
- `ui.air.*`: model, stage, builds, select, absent, rest, sizes, inspector, framestage, text, textcheck, scopemap, ram.
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

**Whole suite**: `tee-view-tests all` 112 of 112 pass after U1's LISTEN re-bless.

**Wanted from the lead**: a simulator "slow time" control would let SLOW show the waves truly slowed. Today SLOW replays
the frame-rate copies at an eighth of their pace.

**Not run**: macOS and Windows builds (CI); the GPU path. **Kit**: no change. FunkGui and FMinus: no issues met.
