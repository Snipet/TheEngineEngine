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
- **Not done**: `tee-bench --profile` not run (Q profiled it: the generic junction path, see Q's notes).
- **Seen, not mine**: combo `every choice and switch of spec.boost / spec.exhaust stepped` fail on
  `boost.intercoolerOut` and `boost.throttleIn` = 0 at 0.01 s (card F's telemetry range).
- FMinus: `0` is not a null pointer (use a flag beside it). Listen to: V8 bank vs `v8-180-degree`; the bypass at 0.3 s.
- **Since (16:20-17:00)**: `tel.exhaust.headerTemperature` (both models) and `scope.ductMid[84]` (dacb9f8).
  `Duct::filtFirst` fixes card T's pack()/extra() passivity finding, set by the builder only where a pass could exceed
  unity, so model 0 and model 1 at its defaults are still identical (57c9876). `spec.exhaust.heatSettle` gives card C
  its settle of the ports' gas and every wall, in both models (859bc7d). Still open: glides; external gates' pipes; the
  heavy diesel's flap (asked the lead what it should be).
- **Since (17:00-17:50), the lead's 16:23 rulings**:
  - (1) Passivity: the exact one-pass gain (`XBuild::passGain`) decides `filtFirst`. The sweep test: 180 pipes,
    largest pass 0.99986, where the reference's order reaches 1.0718. The engine at its defaults is still identical
    on 17 layouts. The harness reference test compares with `passFix` 0 and prints the fix's difference (-47 to
    -58 dB on 6 of 26 runs). The dual X soak passes: 60 s, peak 0.876, the valves travelled 26.2.
  - (2) Flap: `flapKind`, `flapOrifice`, `flapRelief`, `flapTime`, with `Exhaust::setFlap` / `flapFitted` for D2
    (88cb216). Harness: 11.6 kPa open, 655 kPa shut (relief 600), 382 kPa (relief 350).
  - (3) External gates: `Boost::externalGates()` / `gatePipe`, each gate's own pipe. 2 L four 0.116 kg/s through it.
  - (4) Glides: every length, bore and size (0.2 s), with no fade. Kinds, tables and 0<->own still rebuild.
    Measured: the fade never moved, steps 0.3865 against 0.3901 running.
  - (5) Q answered: the cost is in the generic junction path. (7) N1: diameters answered.
    Also added `tel.exhaust.portPressure` for N1.

## The pops' seam: the account's side, for N1 (25 Nov; the lead's plan)

G's half, `exhaust.bang` 1 ("Burned in the pipe"), is on the local branch `g/bang-wip` (8c58a37e). It goes onto the
branch at the next sim window, on the lead's word. The design and its sources are in docs/models/exhaust.md, "Bangs:
heat released in the pipe". This section is what the account (`sim/ecu/overrun.fm`) and combustion's call must do so
that it is fed. G reviews the diff.

**The call.** `Combustion` holds `pop` (= `&exh.pop[0]`; one per bank, 0 and 1). Under `pop[0].burnPipe != 0` (the
exhaust sets it each sample: exhaust.model 1 and exhaust.bang 1), the account hands each burn as

    pop[b].burnEvent(heat, fuel, mass, t, phi, xb, kind, own)

| argument | unit | meaning |
|---|---|---|
| `heat` | J | the fuel that burns times its lower heating value (`kFuelLhv[fuelRow(grade)]`); ≤ 0 is ignored |
| `fuel` | kg | that fuel |
| `mass` | kg | the slug's whole gas: its fuel, its own air, its burned gas (sets the slug's length in the pipe) |
| `t` | K | the slug's gas as it left its cylinder (`cy->t` at its exhaust opening) |
| `phi` | | the slug's equivalence ratio (its fuel × AFR / its air). Outside 0.6 to 1.8 the exhaust burns nothing |
| `xb` | | its burned-gas share (combustion's `dilution[c]`) |
| `kind` | 1, 2 | `BURN_PREMIXED` 1: a charge that left with its own air (a spark cut, a misfire, an overrun charge burned late), a flame runs through it. `BURN_UNMIXED` 2: a rich charge's leftover meeting another's air; the exhaust leaves it out of the sound (Coward and Jones's limits: no flame runs) |
| `own` | 0, 1 | 1: the slug is still burning as it leaves (`cy->burnAt >= 0 && cy->qTot > 0` at `evoNow`). It is lit at once and burns from within; 0: lit by a hot blowdown, the hot gas folding into it |

- **When.** Once for each burn, at the moment it lights. A premixed slug lights when its own cylinder opens its exhaust
  valve still burning (`own` 1). Otherwise it lights at the exhaust opening of a later fired cylinder of the same bank,
  with chance (T − 900)/300 clamped to 0..1 of that cylinder's gas (the account's own ramp, judgement). It waits only
  while still flammable: about 16 ms (kSlugLife: a stoichiometric slug mixing with an equal mass of air passes λ 1.8 at
  0.8 of a 20 ms mixing time; judgement). After that its fuel and air join the unmixed store.
- **Where.** At most 8 events per bank between two of the exhaust's samples (`POP_EVENTS`); the exhaust drains the
  inbox every sample, so in practice one control step's lights.
- **Keep by cylinder.** What a charge lets out unburned is known at its intake closing (`add()` now), but it reaches the
  pipe at its exhaust opening. Keep it by cylinder (fuel, air, x_b, premixed or not) and release it at `evoNow`
  (combustion's `sample()`, the block that writes `ecu->blow[b]`). A charge is premixed when its firing is
  `CHARGE_SPARK_CUT`, a misfire (`!one.lit` and not a fuel cut), or `CHARGE_OVERRUN` with a late remainder. Its
  premixed fuel is min(fuel, air/AFR) of that same charge; the rest of its fuel and air go to the unmixed store.
- **What the account stops doing under `burnPipe`:**
  - the 30 kPa ceiling, `pulse[b]` = 30 kPa (1 − exp(−m/7 mg));
  - the hazard integral (40·hot·m/2 mg per second);
  - firing `pop[b].fire(...)`.
  It keeps the unmixed store's 0.25 s flush. It may keep `bangs` (premixed events) and `total` (fuel handed), so H9's
  integrated test still reads.
- **The gate.** With `burnPipe` 0 nothing changes, to the bit: `ecu.model` 0's dice, `exhaust.model` 0, and Carved.
- **Tests that prove the pair** (G writes them once the seam is in):
  1. Energy: the exhaust's `b.bangGiven` equals the account's handed fuel × LHV, to 1e-4, over a spark-cut limiter.
     Released plus swept equals given is already BangTest's.
  2. A stock fuel-cut limiter, and a lift from 6000 on first.json: no premixed event and `bangHeat` stays 0. The
     sound is the cut firings.
  3. Per event, a spark cut's premixed heat and its peak source exceed a fuel cut's, which has none. The ordering is
     over the rally five against first.json.
- Not in this seam: anti-lag's heat into the turbine (the lead: listed, not built).

**The turbine's books and the bangs (for N2's lane, the lead's flag, 25 Nov).** Under Burned in the pipe, a burn's heat
enters as a volume source at the collector. Its expansion raises the pressure ahead of a turbine: the rally five's
collector reached 4.4 to 5.6 bar mean on anti-lag. `bookLet`, which closes the turbines' mass against what the valves
let out, never sees that heat, and the housing's gas temperature doesn't either.
- bookLet would need the burns' heat in its energy book: the enthalpy into the housing per block, `b.burn[k].released`'s
  increase, alongside the valves' `mE`.
- The housing's inlet temperature would need to rise by that heat over the block's mass flow and cp.
- Not built (anti-lag's turbine drive waits: the lead's ruling).

**Stated bores' floor (X-038, 25 Nov; the lead's option 2).** A stated exhaust pipe bore in (0, 10 mm) is raised to
10 mm by the builder, and `tel.exhaust.boreRaised` / `boresRaised` say which (docs/models/exhaust.md, "Stated
bores"). The intake's twin (X-040: runner 8 mm, snorkel 10 mm) is N1's, with the helper and sites sent. **For card C,
next window (refusal at input in the loader and the inspector):** the FMinus compiler is the owner's and not ours to
change, so no new `fminus::` attribute. The rule lives in the host (a table beside the Spec's lint in
host/src/Fields.cpp) or in an attribute namespace the compiler already passes through.
