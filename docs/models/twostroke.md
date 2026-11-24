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
- **Tuning against the pipe's length.** The pipe's gain (trapped fresh charge over a straight pipe's) is centred at
  11 548, 9627 and 9320 rpm for pipes ×0.8, ×1 and ×1.25. The returning wave's prediction, (192 − 25)° = 6N · 2L/c,
  gives 13 267, 10 614 and 8491: −13 %, −9 % and +10 %, within the 15 % asked. The longest pipe's band hardly moves
  from the default's, so the response to length is weaker than 1/L there; secondary reflections are the likely cause,
  not examined further.
- **On the pipe.** 15.1 N m within 1000 rpm of the band's centre, against 11.2 N m at 0.7 of it and 8.4 N m for a
  straight pipe there.
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
