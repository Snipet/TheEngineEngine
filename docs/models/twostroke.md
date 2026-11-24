# The two-stroke

*Card T ([w2-cards.md](../plan/w2-cards.md), started early by D's agent while K3b was pending). Standalone: the
cylinder, its crankcase and an expansion chamber, on a crank held at a set speed (`tests/sim/twostroke/`,
`tee-tests-twostroke`). Not yet in the engine: no Spec fields appended, no build files, singles only so far.*

## Files

| File | Module | What it is |
|---|---|---|
| `sim/core/twostroke/spec2.fm` | `TwoStrokeSpec` | the fields (cylinder, ports, power valve, crankcase and inlet, scavenging, burn, pipe), a stand-in for the Spec |
| `sim/core/twostroke/cylinder2.fm` | `TwoStrokeCylinder` | the cylinder and its crankcase as gas; ports by the piston; reed or disc; scavenging; the burn |
| `sim/core/twostroke/chamber.fm` | `ExpansionChamber` | header, diffuser, belly, baffle cone, stinger as the reference's `Duct`s (`sim/ref/pipes.fm`, read-only) |

## The model

- **Ports by the piston.** Each port opens when the crown passes its top edge (placed from its duration about bottom
  dead centre through the slider-crank); its area is its width (a share of the bore) times how far below the edge
  the crown is, times a discharge coefficient (0.7, judgement). A power valve shortens the exhaust's duration below a
  speed, blending to the full port by another.
- **Crankcase.** A second volume, its compression ratio a field, pumped by the piston's underside; fed through a reed
  (opening with the drop across it, with a lag) or a rotary disc (a window of crank angle), in series with the
  carburettor's bore (3 % of it open at closed throttle).
- **Flows.** The exhaust port through the reference's `Valve`, solved with the header's pipe. The transfers and the
  inlet are compressible orifices between volumes, **stepped implicitly**: an explicit step of a √Δp law between two
  volumes chattered for ever (4.6 g/s back and forth through the transfers of an engine at rest, holding the pipe 23
  to 33 Pa off the air); the implicit step's flow is at most what evens the two volumes.
- **Scavenging.** A blend of perfect displacement and perfect mixing (`mixing`, 0.55 by default), with a short-circuited
  share (10 %) leaving first. What a returning wave pushes back through the exhaust port is the last gas out, first:
  a column of a tenth of the swept volume's worth of air by the port, with its fresh share. The first version used
  the whole exhaust event's mean, mostly blowdown gas, and the plugging pulse recovered almost nothing.
- **Burn.** At the spark, the fuel in the fresh charge, burned by the reference's Wiebe curve. The share that burns
  is the reference's law of dilution (`sim/ref/engine.fm`, charge: half at a burned share of 0.56, width 0.035),
  without its random scatter. Unburned charge stays fresh for the next cycle.
- **Energy.** The reference's filling-and-emptying bookkeeping, with the walls' heat at the reference's coefficient.
  The crankcase is adiabatic.
- **The chamber.** Each cone is `coneSteps` ducts stepped in bore, joined by the reference's junction (drive through
  the impedances in series, Borda–Carnot loss for an expansion, half of (1 − a/A) for a contraction, on the slow
  part). The stinger's end is open (inverted, less the exit's loss). One gas temperature for the whole pipe (800 K in
  the tests: c = 557 m/s).

Defaults: a 125 cc racing single (54 × 54.5 mm, exhaust port open 192°, transfers 130°, crankcase ratio 1.4), with a
pipe 0.73 m to the baffle cone's middle. This is judgement from published tuning practice, not a particular engine.

## Measured (tee-tests-twostroke, 96 kHz)

- **Where the plug lands.** Traces at 9000 rpm: blowdown peaks some 24° after the exhaust port opens; the diffuser's
  suction arrives while the transfers are open; the baffle cone's plug comes back 144° later (2.67 ms; 2L/c =
  2.62 ms) and pushes gas back in before the port closes.
- **Tuning against the pipe's length** (card T's open finding, settled). The handoff found the band moved less than
  1/L with the pipe's length. The cause was the measurement, not the pipe: the gain was taken over a straight pipe that
  was one duct, and a duct is cut at 0.98 m, so the straight pipes for the ×1 and ×1.25 chambers (1.07 and 1.34 m) were
  the same 0.98 m pipe, whose own tuning (its open end's rarefaction) was divided into the gain. The straight pipe is
  now cut into equal sections, and the band is measured against an **endless pipe** (the far end sends back none of
  what moves, but its mean pressure is the air's: a matched end would also hold z Q of back pressure, 0.3 bar at
  15 500 rpm, and bias any baseline against speed). Centres for pipes ×0.8, ×1, ×1.25: 13 900, 11 753 and 9 125 rpm
  (before: 11 548, 9 627, 9 320); the returning wave says 13 267, 10 614, 8 491: +4.8, +10.7, +7.5 %. The centres'
  ratio ×1.25/×0.8 is 0.656 against 1/L's 0.640 (before: 0.81). The plug's own timing scales exactly: the port's
  pressure at closing peaks at 13 500, 11 000 and 8 500 rpm (N L within 2 %). Also tried and ruled out: the cones'
  junction losses on the whole drive instead of its slow part moved nothing by more than 2 %.
- **On the pipe.** Strictly periodic at every speed (no spread from cycle to cycle). The chamber's torque (×1) rises
  48 % in 1000 rpm, from 10.6 N m at 8500 to the band (8750 to 11 000 rpm, the plug at the port as it closes); best
  15.5 N m at 10 750; 14.2 N m within 500 rpm of it against 9.6 at 0.7 of that speed and 11.2 for a straight pipe of
  the same length. A second, lower lump at 12 000 to 13 500 rpm (13.6 N m) with a dip between (11.6 at 11 500), as
  the plug passes the port's closing; above 14 500 rpm the chamber is no better than the straight pipe.
- **Cone steps.** The band's centre moves 2.5 % from 1 to 2 steps a cone, 1.2 % from 2 to 3, and under 0.3 % after
  that: 3 steps settle the tuned speed, and 8 is the default. The sound's dependence on the steps was not measured.
- **Four-stroking.** At closed throttle and 7000 rpm, fire and misfire alternate on every cycle (purity 0.39), from
  the dilution law and the unburned charge left behind, with nothing scripted. At 5000 rpm the model settles to a
  steady partial burn; from 11 000 rpm to a steady weak one. At full throttle there are no misfires.
- **Mass** in through the inlet, less mass out through the exhaust, equals what the cylinder and crankcase hold, to
  2e-6 of the flow. **Passive** stopped: from 4 bar, the port settles within 0.4 Pa of the air in 3 s (walls at the
  air's temperature). **Finite** under a 60 s soak of random speeds and throttles (peak 64 bar).
- **Level.** Some 15 N m on the pipe from 125 cc (about 15 kW at 9600 rpm): a mild 125, below a racing one's 30 kW.
  Not tuned.

## Not done

- Singles only: twins and triples (each cylinder its own pipe) are wiring.
- No Spec fields, no builds, no engine integration (it waits for K3b and card D's integration).
- No sound was listened to. Nothing in `sim/exhaust/` was touched.
