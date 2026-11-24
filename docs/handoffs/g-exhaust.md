# G: the exhaust builder (standalone handoff)

Card G ([w1-g-exhaust.md](../plan/w1-g-exhaust.md)), branch `claude/awesome-gates-ntdb58`. `exhaust.model = 1` is built
and proven on made-up port flows; **integration waits for K3b**. Nothing outside card G's paths touched. Model, every
coefficient's source and what is judgement: [docs/models/exhaust.md](../models/exhaust.md).

**Done** (`tee-tests-exhaust`, ctest `exhaust.*`, 12 cases; harness `tests/sim/exhaust/harness.fm`, the reference's
exhaust copied alone in `refexhaust.fm`). Files: `sim/exhaust/{network,slots,ends,heat2,buildspec}.fm`.
- Test 2 (`model 1 at its defaults is the reference's exhaust`): identical to the bit, 17 layouts + 3 dial sets.
- Test 3 (`every random topology is passive`): 50 topologies; line energy never up > 2.3 % per 5 ms; gone < 0.71 s.
- Test 4 (`every element kind conserves the flow`): 20 kinds, within 0.0073 %.
- Tests 5-9 (`CharacterTest.cpp`): 180-degree headers cut the V8's off-order lines 24-27 dB; boxer unequal +16 dB;
  X 1.8-2.5 dB over H above 250 Hz, both shrink the tails' difference; quarter-wave notch -38.5 dB, Helmholtz -21.2;
  mufflers IL 22.2 / 15.9 / 1.8 dB and back pressure in the card's order; bypass open +16.7 dB; no converter 3.2 vs
  4.6 kPa and +3.8 dB above 2 kHz; zoomies: one end a cylinder.
- Test 10, standalone (stand-in housing): passive; downpipe swings 3 dB less. Test 11 standalone (45 random
  topologies rebuilt while running, 3 min): finite, bounded, nothing left 3 s after. Also the Scope, meters.

**Not done**: tests 1, 2 and 11 in the engine, 10 with F's turbine (need K3b); `content/builds/exhausts/` (Spec).

**Cost** (dev build, R = 2549 ns by `tee-bench --quick`): 54-62 ns a pipe a sample with its junctions; V8 reference
topology +0.09-0.12 R over the reference's own exhaust (21 pipes); dual with X, stubs and valves +0.31 R (32 pipes):
**over the card's 0.25 R**, nearly all the 11 extra pipes. After K3b `Duct` is mine: a pipe with no wall, packing or
scattering could skip its filters' loop and tracker (model 1 only); or a quality tier drops the bypass's tubes.

**Turbine seam (F)**, per sample: `exhaust.read()`; cylinders; `exhaust.prepare()`; then for each housing h
`setHouse(h, Boost::housing(h, houseDrive(h), houseAdmit(h), houseT(h)))`, each side s
`setDown(s, Boost::downpipe(s, downWave(s), downZ(s)))`, each gate t `setGate(t, Boost::gatePipe(t, gateWave(t),
gateZ(t)))`; then `exhaust.step(q)`. The integrator fills `XEngine.housing[c]` from `Boost::housingOf` and `gates`, and
zeroes the reference's restriction (`aspUse`). One change to F's text: the drive includes each pipe's junction loss
(`sum 2a/z - hold`); each pipe takes `p - a` plus its loss's share. `downZ` includes the converter in series.
**H**: `XBuild::pop(int bank, float amp)`, `float gasTemperature(int bank)`, as H asked.

**Findings**: (1) the reference clears pipes before sizing them, so a steepening pipe starts timed for its old delay
and leaks its first pulse as a ramp (energy x40 in 160 samples): the builder settles after sizing (`XNet::settle`);
model 0 keeps the reference's start. (2) A 1-D model's resonators must carry their end corrections in their length
(built without, the Helmholtz rang 1.3x high). (3) A bypass joined straight at a chamber is shorted by its volume.

**Outside my paths**: none needed. **FMinus issues**: none met. **Listen to** (after integration): V8 bank vs
180-degree headers at 2500-3500 rpm; the bypass opening at 0.3 s travel (level overshoot, no click measured).

## Integration (appended)

Card G, branch `claude/awesome-gates-ntdb58`, pushed at cf267fb.
- **Done in the engine** (`tee-tests-exhaust`, `exhaust.*`). Test 1: the null test, every pair identical. Test 2
  (`model 1 at its defaults is model 0 in the engine, every layout`): identical to the bit on 17 layouts, 8 s of free
  revs. Test 10 (`the turbine's housing`) with F's machine: 2 L four 65.5 kPa, V8 twin 46.2 kPa; downpipe swings
  16-17 dB less than the collector. Test 11 (`random exhaust topologies changed while the engine runs`): 10 min,
  finite, nothing left after the key is off. `every exhaust build plays`: 8 part builds in `content/builds/exhausts/`.
- **Wiring**: `Exhaust` dispatches on `spec.exhaust.model` itself (sim/README.md, Exhaust). engine.fm: only my
  calls' arguments (`setup` takes the section, Telemetry, Scope, `&boost`; `heatUp` takes rpm and throttle), and F's
  stand-in is skipped while `exh.turbinesFed()`. Turbine seam: as F wrote it, the drive including each pipe's loss hold.
- **Spec** (spec.exhaust, appended; `crossKind`, `mufKind` renamed from the card's names), **telemetry**
  (tel.exhaust: model, pTurbine, pConverter[2], pMuffler[2], gasTemperature[7], wallTemperature[7], mach[7], valve[2],
  ends, endLevel[24], endTag[24], endSlot[24], flame) and **Scope** (exhaustPoints, exhaustX[64], exhaustPressure[64],
  exhaustRefreshes, ductUsed/Length/Near/Far[84]): final; sent to U3 with the slot numbering.
- **Cost** (`tee-bench --engine`, free revs, least of 3, this container): model 0 1.04-1.05 R. Model 1 with the
  reference's topology 1.22 R (+0.17 R). V8 with an X, quarter-wave stubs and valved mufflers 1.48 R against 1.04 R
  at model 0 (+0.44 R). **Over the card's 0.25 R**: the extra pipes, plus `shape()` every 16 samples (as the
  reference's `pipes()`). Proposed for the performance package: shape only when something moved; skip bare pipes' filters.
- **Not done**: glides (every builder field change rebuilds under the fade, 6 + 12 ms); the external gates' pipes not fed
  (Boost lists no count); `tee-bench --profile` not run; the full suite not run on cf267fb (the null test, exhaust and boost ran on
  the tree merged just before).
- **Seen, not mine**: combo `every choice and switch of spec.boost / spec.exhaust stepped` fail on
  `boost.intercoolerOut` and `boost.throttleIn` = 0 at 0.01 s (card F's telemetry range).
- FMinus: `0` is not a null pointer (use a flag beside it). Listen to: V8 bank vs `v8-180-degree`; the bypass at 0.3 s.
