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
- Test 10, standalone form (stand-in housing: a volume and a linear nozzle): passive; downpipe swings 3 dB less.
- Also: Scope (64 points port to tail), Mach and temperatures a section, back pressures, valve travel, end levels.

**Not done**: tests 1, 2-in-the-engine, 10 with F's turbine, 11 (all need K3b); `content/builds/exhausts/` (needs the
Spec fields in `sim/spec/exhaust.fm`).

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
