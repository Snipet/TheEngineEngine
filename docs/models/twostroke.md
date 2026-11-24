# The two-stroke

*Card T ([w2-cards.md](../plan/w2-cards.md)). A crankcase-scavenged two-stroke of one to four cylinders, each with its
own expansion chamber, beside the four-stroke cylinder model. `spec.engine.cycle` 1 ("Two-stroke") on a custom engine
(`spec.engine.custom`, its bore, stroke, rod, masses, friction by part and redline) runs it; at 0, the default, none of
it is read.*

## Files

| File | Module | What it is |
|---|---|---|
| `sim/spec/engine.fm` (card T's marked block) | `EngineSpec` | `cycle` and the `ts*` fields: cylinders, firing, ports, power valve, crankcase and inlet, scavenging, burn, pipe, silencer |
| `sim/core/twostroke/twostroke.fm` | `TwoStroke` | the engine: cylinders with their firing offsets, a pipe each, the pipes' gas temperature, the power valve, the sound out, the reports |
| `sim/core/twostroke/cylinder2.fm` | `TwoStrokeCylinder` | the cylinder and its crankcase as gas; ports by the piston; reed, disc or piston port; scavenging; the burn |
| `sim/core/twostroke/chamber.fm` | `ExpansionChamber` | header, diffuser, belly, baffle cone, stinger, silencer as card G's `Duct`s (`sim/exhaust/duct.fm`) |
| `content/builds/engines/two-stroke-*.json` | | a motocross 125, a 250 road twin, a 500 piston-port triple, a 100 cc disc kart, a 50 cc utility engine |
| `tests/sim/twostroke/` | `tee-tests-twostroke` | the rig (`rig2.fm`, a crank held at a set speed) and the engine-level bench (`Bench.h`) |

## The model

- **Ports by the piston.** Each opens when the crown passes its top edge (placed from its duration about bottom dead
  centre by the slider-crank). Its window is a rectangle `width` (a share of the bore) with rounded top corners
  (`*Radius`), its open area tabulated against the crown's depth below the edge (64 steps, at setup), times a discharge
  coefficient (0.7, judgement). A **power valve** is a blade straight across the exhaust's top: it lowers the edge to
  `tsValveDur` below `tsValveFrom` and is fully up by `tsValveTo`, moving at an actuator's pace (0.1 s across).
- **Crankcase.** A second volume pumped by the piston's underside (`tsCrankcaseRatio`). The inlet: a **reed** (a sprung
  petal, `tsReedHz` and `tsReedDamping`, pushed off its seat by the drop, fully open at `tsReedFull`, landing on seat and
  stop without bouncing; it shuts against a flow back), a **rotary disc** (a window of crank angle), or a **piston port**
  (the skirt uncovers it about top dead centre). In series with the carburettor's bore (3 % of it at a closed throttle).
- **Flows.** The exhaust port through `Valve` (`duct.fm`), solved with the header's pipe. The transfers and the inlet are
  compressible orifices between volumes, **stepped implicitly** (an explicit step of a square-root law between two
  volumes chatters for ever: 01-rules.md section 4).
- **Scavenging.** A blend of perfect displacement and perfect mixing (`tsMixing`, 0.55), a short-circuited share (10 %)
  leaving first. What a returning wave pushes back through the exhaust port is the last gas out, first (a column of a
  tenth of the swept volume's worth of air). The trapped charge's purity follows.
- **Burn.** At the spark, the fuel in the fresh charge, by the reference's Wiebe curve; the share that burns is the
  reference's law of dilution (half at a burned share of 0.56, width 0.035), without its scatter. Unburned charge stays
  fresh for the next cycle. The ECU's cut (limiter, fuel cut, off) takes the spark away.
- **Energy.** Filling and emptying, each flow carrying its gas's enthalpy; the walls' heat at the reference's
  coefficient; the crankcase adiabatic. The temperature is solved from the energy in closed form with gamma's line in
  temperature, so the energy is kept exactly (the reference's update, gamma at the old temperature, lost 4 % of the burn).
- **The chamber.** Each cone is `tsConeSteps` ducts stepped in bore, joined by the reference's junction (drive through
  the impedances in series; Borda-Carnot loss for an expansion, half of (1 - a/A) for a contraction, on the slow part).
  The open end inverts what arrives, less the exit's loss. A **silencer** (off by default; on in the builds): a core
  wrapped in packing, or a can (a bore over twice the stinger's) with an outlet tube; the packing `Duct::pack`'s shelves with card G's `filtFirst` (the shelves before the steady-flow tracker: passive).
- **The pipes' gas** follows the gas leaving the ports, mass-weighted over 0.2 s, with a 1.5 s time constant (the
  walls' warming): some 680 K at full load on the 125 after 3 s. The tuned speed rises as the pipe heats.
- **In the engine** (sim/engine.fm's `twoStrokeTick`, `scopeTwo`; tel.engine.ts*): the ECU's throttle plus its idle valve as a slide's idle stop (12 % of the
  bore at most, judgement), the ECU's cut as the spark's; the crank, friction, inertia and starter are the custom
  engine's. The stingers' radiation (the rate their flow out changes) goes to the mix in place of the tails'; the
  carburettors' draw to the intake's voice. The Scope's cycle is 360 degrees; its valve lifts carry the exhaust and
  transfer ports' open shares; `portIntake` carries the crankcase's pressure; `inletOpen` the inlet's open share; the work loop (core/work.fm) is fed per cylinder on its 360 degrees. The cycle switch is held to the fade's quiet as the model switches are.

## Measured

**The tuned length** (`PipeTest`, the rig, the pipe's gas held at 800 K, c = 557 m/s). The handoff found the band
moved less than 1/L. The cause was the measurement: the gain was taken over a straight pipe built as one duct, cut at
0.98 m, so the x1 and x1.25 chambers were divided by the same 0.98 m pipe and its own tuning. Now the straight pipe is
cut into sections and the baseline is an **endless pipe** (no reflection of what moves, the air's mean pressure; a
plain matched end holds z Q of back pressure, 0.3 bar at 15 500 rpm). Band centres for pipes x0.8, x1, x1.25: 13 033,
10 466, 8 044 rpm against the returning wave's 13 267, 10 614, 8 491 (-1.8, -1.4, -5.3 %); before the fix 11 548,
9 627, 9 320. The plug's arrival at the port's closing scales within 2 %. Ruled out: the cones' junction losses
(whole drive against slow part: under 2 %).

**On the pipe**: a step of 42 % in 1000 rpm into the band; 12.0 N m within 500 rpm of the best against 9.2 at 0.7 of
that speed and 6.4 for a straight pipe of the same length (indicated, no friction). The band has two lobes (the plug at
the port's closing, then a second near 12 500 rpm), each speed strictly periodic.

**Cone steps**: the tuned speed moves under 1 % from 3 steps a cone; 8 is the default.

**The builds** (`EngineTest`, the engine at 48 kHz, full throttle on the engine dyno, brake power):

| Build | Best | Where | The class (published) | Share |
|---|---|---|---|---|
| 125 motocross single | 11.6 kW | 11 524 rpm | some 28 kW near 11 500 | 41 % |
| 250 road twin, 180 degrees | 18.8 kW | 8 504 | 26 kW at 8 500 (Yamaha RZ250, 35 PS) | 72 % |
| 500 triple, piston ports | 28.5 kW | 7 497 | 44 kW at 7 500 (Kawasaki H1, 60 PS) | 65 % |
| 100 cc kart, rotary disc | 9.1 kW | 14 051 | some 20 kW near 14 500 to 17 000 | 46 % |
| 50 cc utility | 2.0 kW | 9 525 | 2.2 to 2.6 kW at 9 000 to 9 500 | 82 % |

Each peaks within 10 % of its class's speed. The output is low for racing engines because the model's crankcase pump
delivers less: delivery ratio about 0.88 and trapping efficiency about 0.65 on the 125, against 1.1 to 1.2 delivered on a
racer, which gets it from the inertia of the inlet tract and the transfer ducts (their ram), left out here. Doubling the
carburettor's and reed's areas adds only 3 %. **This is the main thing not done**: an inlet duct (a `Duct` from the air
to the reed, its mouth the carburettor) and the transfers' gas inertia. Port timings and pipes in the builds are judgement
for each class (Jennings, *The Two-Stroke Tuner's Handbook*; Blair, *Design and Simulation of Two-Stroke Engines*), not a
maker's drawings; the published powers are the makers' claims.

**Four-stroking**: at idle every build misfires on 80 to 84 % of its cycles (closed throttle at 7000 rpm on the rig:
fire and misfire alternate every cycle, purity 0.38), from the dilution law and the unburned charge left behind, with
nothing scripted; held on the pipe at full throttle, none. At idle that is nearer eight-stroking than four.

**Kept**: mass to 1e-7 of the flow; energy to 0.01 % of the burn's heat (the walls, the work, the enthalpies in and out).
**At rest**: from 4 bar, everything settles (port within 0.4 Pa, flows under 5e-6 kg/s in 3 s); at the air's pressure,
flows under 1e-5 kg/s (float's resolution at 1 bar). **Finite** under a 60 s soak of random speeds and throttles.

**Inlets** (the rig, full throttle): a disc delivers 18 % less than the reed at 4000 rpm (it spits back) and the same at
13 000; a 200 Hz reed 5 % less than a 550 Hz one at 13 000. **Power valve** (170 degrees closed): +37 % torque at 4000
rpm, +6 % at 6000, none in the band.

**Spectrum** (the rig, what the outlet radiates): every line is a multiple of the firing frequency (once a turn a
cylinder; in a twin or a triple the other multiples of the turn's frequency are 100 dB down). The strongest line is
**not** the firing frequency: the radiated sound is the rate the flow changes (+6 dB an octave), and a chamber is a
high-pass for waves (its cones are long against 1 kHz) while its belly and stinger are a 55 Hz Helmholtz low-pass for the
pulses; the header's quarter wave (some 870 Hz for 0.16 m) is the strongest line, the firing frequency 7 to 27 dB under
it. On the utility engine (a can, no chamber) the firing frequency is within 9 to 16 dB of the strongest (the 4th or 5th
multiple, where the can's low-pass begins). This departs from the card's wording; it is the physics of a radiating
monopole and the reason racing two-strokes "ring".

**Cost**: a single with its chamber (8 steps a cone, 19 to 21 ducts) 0.28 R on the rig (500 ns a sample against R's
1800); in the engine the 125 takes 0.10 of a core at 48 kHz, the triple 0.25.

## For a person to listen to

- Idle on every build: the irregular firing (misfire after misfire), the pipe's ring between firings.
- The 125 and the twin pulling through 8000 to 11 000 rpm: the step as it comes on the pipe; the power valve opening.
- Overrun from the redline with the throttle shut: four-stroking coming back, and whether it sounds like a two-stroke
  burbling or like a fault.
- The triple's 120-degree beat; the twin's 180; the kart's scream at 14 000; the utility's buzz behind its can.

## Not done

- The inlet tract and transfers' inertia (the main cause of the low output, above).
- Nobody has listened. A shared chamber for several cylinders, a boost port, auxiliary exhaust ports as their own
  windows, and the oil's smoke are left out.
