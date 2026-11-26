# The outside air's pressure in the engine: a design (D, 26 Nov; the next window's item 15, no code yet)

Today `spec.env.pressure` (the Air pressure dial, "lower at altitude") is read only by the vehicle's drag
(sim/vehicle/car.fm:393). The engine uses the literal 101325 wherever it needs the outside (the lead's read and mine,
26 Nov). This design states every reader, the null, what a boost target means at altitude, predictions, and what the
interface shows. Nothing under sim/ is changed by it.

## 1. Every reader of the outside's pressure (the tip after the bake, d586682c's tree)

**A. True readers of the outside: they take `pAmb` (the spec's field, as Engine hands it).**

| where | what it is | owner |
|---|---|---|
| engine.fm:301, :1007 | `intk.pBox` = 101325 (+ the reference's boost): the air box | D (Engine), with N1 |
| engine.fm:477, :502 | a cylinder's and the plenum's pressure at configure (at rest: the outside) | D |
| engine.fm:528, :1694 | `boost.applyMachine(..., 101325, tAmb)`: the machine's `pOut` (machine.fm:292) | D hands it; N2's machine already takes it |
| engine.fm:540, :1197 | the two-stroke's configure and step (its crankcase and ports against the outside) | D hands it; T's twostroke.fm:204 (`rhoAir`) |
| engine.fm:1389 | `work.trapped(..., 101325, tAmb)`: the trapped charge against the outside's | D |
| engine.fm:1413 | the piston-slap click's pressure over the outside | D |
| engine.fm:1570 | `showMap`: the manifold as gauge | D |
| engine.fm:1998 | `tsCrankcase` with no two-stroke: the air's | D |
| core/cylinder.fm:265 | a cylinder's pressure at rest | D |
| core/cylinder.fm:414-415 | the valves' port pressures: the pipe ends' waves are over the outside | D, with G and N1 (their waves' reference) |
| core/cylinder.fm:471-474 | the rings' leak, to a crankcase at the outside | D |
| core/cylinder.fm:581 | the gas torque: the piston's underside at the outside | D |
| core/twostroke/cylinder2.fm:424 | as cylinder.fm:414 | T |
| core/wallheat.fm:62 | the walls' pressure at rest | D |
| intake/intake.fm:133, :266, :306, :328, :338 | the runners' ends, the plenum's and the box's gauge, all over the outside | N1 |
| boost/boost.fm:353 | model 0's absolute boost: 101325 + 1e5 x boost | N2 (model 0: stays 101325, the null) |
| exhaust/exhaust.fm:911, :1132, :1785 | the port gas's density, the back pressure, the port pressure's telemetry (over the outside) | G |
| exhaust/slots.fm:1222-1225, :2299 | the muffler's loss against the outside; the turbine's outlet | G |
| exhaust/duct.fm:347-348, :596, :678, :687 | a pipe's impedance (rho c from gamma p / c, the gas at the outside's pressure), its steepening and its quasi-steady loss | G: QUESTION for G. A pipe's mean pressure is the outside's plus its flow's. At 70 kPa its rho c falls by 0.69, and its waves are relative to the outside. Is `z` to take `pAmb`, or a mean pressure the pipe already tracks? |
| combustion/heavy.fm:152-153, :205, :258 | the diesel's start pressures, its full fuel (air density at 101325 + its boost) and the back pressure's density | the diesel's owner |
| ecu/ecu.fm:317 | `in->ambient = 101325`: the ECU's barometer | H |
| vehicle/truckair.fm:69 | the truck's air compressor's intake | I |

**B. References that stay 101325 (corrections or calibrations stated at the standard atmosphere, not readings of
the outside):**

- boost/machine.fm:1567, :1596 and spec/telemetry.fm:99: the compressor's corrected flow and speed, at 101325 Pa and
  293.15 K by definition.
- combustion/knock.fm:210: the knock table's pressure index (p / 1 atm). It's a table's axis; the cylinder's
  absolute pressure is what knocks, not the outside's.
- ecu/ecu1.fm:195 (`loadRef = manifold / 101325`: "the reference's formulas go by 1 atm"), and
  combustion/combustion.fm:118, :145, :162, :170, :245, :257, :314, :325, :352, :361, :418, :432 (`pPlenum / 101325`:
  the manifold as a fraction of an atmosphere, the reference's load axis for advance, burn duration and the
  burnable share). These stay 1 atm: they're the calibration's load axis, the same absolute manifold giving the same
  spark at any altitude, as a real ECU's MAP-indexed tables do. To confirm with C (combustion): is any of them meant
  as "over the outside"?
- exhaust/slots.fm:2324: a laminar flame speed's pressure exponent over 1 atm (a correlation's reference).
- exhaust/exhaust.fm:596, :609, :616 and slots.fm:1100-1202 (`steep = 0.87 / 101325`): a steepening coefficient's
  normalisation. QUESTION for G: a reference, or the pipe's mean pressure?

## 2. The null

- **Model 0 keeps 101325 everywhere, to the bit.** The reference's own (sim/ref/) is untouched; Engine hands
  `pAmb = spec.env.pressure` only to the readers of section A that run under a model (each part's model switch on),
  and 101325 to model 0's.
- **At the field's default every archetype is bit-identical to today.** The default is 101325.0f, the same float as
  the literal, and every reader takes it unchanged (no `pAmb / 101325` scale that rounds; where a ratio is needed, the
  code keeps today's expression with `pAmb` in the literal's place). So the next bake reproduces its tables, and
  anything else is a fault in the change. The test: every archetype's dyno point and the null suite before and after,
  bit for bit.

## 3. What a boost target means at altitude

- **Today:** gauge. The gate's signal is `v1.p - pOut` (machine.fm:1281); `gateSpring` and `target` (spec/boost.fm:68-69:
  "the boost at which the gate opens") are compared with it, and the design point's ratio is `(pOut + asked) / pOut`
  (machine.fm:466). With `pOut` fixed at 101325 the distinction never showed.
- **What it should be:**
  - The **spring** stays gauge. A wastegate actuator's diaphragm has the charge on one side and the outside air on
    the other, so a spring-only gate holds the same boost over the outside at any altitude, and the absolute
    manifold falls with the outside. This is recalled, not sourced; a source (an actuator maker's or a turbo
    handbook's line) is to find.
  - The **target** given by an ECU should be absolute: the controller reads an absolute MAP sensor and holds a
    manifold pressure, duty-cycling the gate. So a controlled turbo makes up altitude until its compressor's speed or
    ratio limit; that's the usual account of why turbo engines lose little power at altitude. Also recalled; the
    source is to find (a controls text, or a maker's altitude note).
  - So `spec.boost.target` is restated as absolute, with a migration (today's builds' targets + 101325, the same at
    sea level, so bit-identical at the default), or kept gauge with a new `targetAbsolute` switch. I'd take the
    restatement: one meaning, and the label says "absolute".
- **What each shipped build would do at 70 kPa:** spring-only builds hold the same gauge boost, so their absolute
  manifold falls by 31 kPa. ECU-targeted builds (ecu.model 1 with a target) under an absolute target ask the
  compressor for a pressure ratio 1.45 times today's at the same manifold. Where that's past its map (speed or surge
  line, or the turbine's energy at low rpm), boost falls short and the over-boost and the knock tables are unchanged.
  The list of which archetypes are spring-only and which targeted is to read from content/ in the window (a probe,
  not code).

## 4. Predictions, before any measurement

- **Naturally aspirated, full load.** The trapped charge goes with the plenum's pressure, so indicated work goes about
  in proportion to it; friction doesn't. Today at 3000 rpm full load (tee-dyno, 26 Nov):
  - default V8: IMEP 12.34 bar, PMEP 0.30, FMEP 1.46, BMEP 10.57;
  - economy four: IMEP 13.40, PMEP 0.17, FMEP 1.31, BMEP 11.90.

  At 70 kPa (0.691) the indicated parts fall to 8.53 and 9.26 bar. With pumping and friction unchanged, BMEP is 6.77
  and 7.78 bar: **-36 % and -35 % brake torque against -31 % pressure.** (The rings' term, fricPeak x the peak
  pressure, falls a little with it under the physical friction: a few hundredths of a bar less friction; the
  reference's friction doesn't.)
- **A wastegated turbo (spring only):** below the spring's boost the same as NA in proportion (-31 % absolute in the
  manifold). At and above it the gate holds the same gauge boost, so the absolute manifold is 31 kPa lower: at a 1 bar
  spring, 1.70 against 2.01 bar absolute, so some -15 % indicated (and more brake). Under an absolute ECU target the
  loss is near zero up to the compressor's limit, then the shortfall grows with rpm. Where that limit falls on each
  build is the measure's.
- **The idle:** the throttle passes the idle's air at a lower box pressure. A choked throttle's mass flow goes with
  the upstream pressure, so the idle valve opens some 1/0.69 = 1.45 times further for the same air. Where the valve's
  authority runs out (`intk.aBypass` at its stop), the idle sags and the ECU's spark trim takes the rest; the idle
  target holds if it isn't out of authority. Prediction: the default V8 and the economy four hold their idles;
  a big-cam engine (the V10) near its valve's limit may not.
- **The cold start at 70 kPa:** less air a cranking stroke (0.69) and a lower compression pressure; a spark engine
  starts (it fuels to its air); a diesel's compression temperature falls (the polytropic end temperature depends on
  the ratio, not the inlet pressure, but its heat loss share rises with the lower density): later ignition, more
  cranking. Prediction: the large diesel cranks longer by some tenths of a second; no build fails to start.

## 5. What the interface shows

- **Meanwhile:** the Air workspace adds the document's pressure to gauge figures to print absolutes
  (AirWorkspace.cpp:519-541, AirModel.cpp:228). At any setting but the default that prints an absolute the engine
  never had. U1 is asked by the lead for an honest line now (the pressure dial "acts on the car's drag only").
- **After:** the engine's own absolute pressures come from telemetry (tel.exhaust.portPressure and the manifold's)
  with `pAmb` beneath them. The workspace prints the absolutes the engine reports, and its own addition is removed:
  part of this change, U1's file, asked of U1 by name.

## 6. The sound at thin air (the lead's addition, 26 Nov)

A source that radiates into the air, a pipe's open end or a vibrating surface, makes a sound pressure in proportion to
the outside air's density for the same motion. A monopole's far field is rho0 / (4 pi r) times dQ/dt; a surface's is
about rho0 c times its velocity. So at 70 kPa and the same temperature a real engine is quieter at the pipe by
about the density's ratio, 0.69, some -3.2 dB, before any change in what flows. Today nothing in the sound reads the
outside's density or pressure:

| source | where | what it takes today | at thin air it should |
|---|---|---|---|
| the tailpipes' radiation (the builder, model 1) | exhaust/network.fm:571-586 (`solveOpen`: `rad = dq/dt` of the end's volume flow, plus the jet's noise `un^3`) | no density: the listener's gain is a constant (listen.fm:735 for the intake's; the exhaust's level fit, listen.fm:809) | x rho0 / rho0,ref; the jet's noise (Lighthill, u^8 rho0 over c0^5, recalled) by the same ratio to first order |
| the open end's reflection and loss | exhaust/ends.fm:69-86 (`design`: the end's correction from tEnd / tAmb; `kOut`, `kIn` from the gas's rho) | the gas's density, from duct.fm:348's 101325 | G's question above (the pipe's own pressure) |
| the reference's tailpipes (model 0) | exhaust/exhaust.fm:1385-1406 | no density | unchanged (model 0: the null) |
| the intake's mouth (model 1's air box) | intake/airbox.fm:58 (`rho = 1.204`, the Helmholtz neck's mass and the box's compliance), :127 (`sound = dq/dt x 0.02`) | a fixed 1.204 kg/m^3 | the box's rho from pAmb and tAmb (its resonance goes with c, not rho: unchanged to first order); the radiated level x rho0 / rho0,ref |
| the mechanical voices (sim/sound/*: valve gear, gears, fuel, starter, fan, chain) and the listener's bay and cabin (sim/listen/) | listen.fm:1001 (`sc.in[LS_BAY]`), the voices' own gains | no density | a surface's radiation x rho0 c / (rho0 c)ref: at 70 kPa, the same temperature, 0.69; the cabin's own path unchanged |

**Ruled (the lead, 26 Nov): the radiated level takes the PRESSURE's share alone.** One factor, `pAmb / 101325`, on
what radiates into the outside (the tailpipes, the mouth, the bay's surfaces), applied where the listener sums them
(listen.fm: L's file, by L's hand or leave), under the new models only, and exactly 1.0 at the default, so the default
stays to the bit.

**The temperature's share of the density is NOT in this item, and is not to be added to it by tidiness.** It is its
own item after this one: it changes what every build stated at a temperature other than 300 K sounds like today (some
1 dB at the range's ends), which is a change for the owner's ears and a bake of its own. Do not key the factor on
density (pAmb / (R tAmb)) in this item.

**The prediction to test (the default V8, full load, 3000 rpm, at 70 kPa against 101.3 kPa, the same temperature):**
- The radiation factor alone: -3.2 dB (20 log 0.691) for the same flow at the tailpipe.
- The engine's own flow: the mass through it falls about as the pressure (0.69), but the gas at the tailpipe is also
  0.69 as dense, so its volume flow and its pulses' volume velocity (the wave's pressure over the pipe's impedance)
  stay about the same, IF the pipes' impedance takes `pAmb` (G's question, duct.fm:347). Then the expected level is
  some -3.2 dB in all.
- If the impedance stays at 101325 (G answers "a reference"), the volume velocity falls with the pulses' pressure
  (0.69), and the expected level is some -6.4 dB.
- So the measured level settles G's question as well as the factor: a result near -3 dB, or near -6, says which the
  pipes are doing. Anything far from both is a fault to chase before the bake.

The listener (sim/listen/) is L's; the voices are M's; the air box is N1's. Each takes the factor from Engine, as
`pAmb` is handed in section 1.

## Order of work, when the window opens

1. The list of section A confirmed with each owner (G's two questions; C on combustion's load axis), by message.
2. `pAmb` threaded from Engine to each reader, model-gated; the bit-identity test at the default (every archetype,
   the null suite).
3. The boost target's meaning (section 3) to the lead, with its sources, before it changes.
4. The predictions of section 4 measured at 70 kPa on the named builds, against what's written here.
5. The interface (section 5) with U1.
6. The sound's factor (section 6), with L, M and N1, after the lead's ruling on pressure alone or density.
