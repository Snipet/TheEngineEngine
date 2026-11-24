# Forced induction

Turbochargers and superchargers as machines: a compressor with a map, a turbine driven by the exhaust, the shaft
between them, the charge pipes and intercooler, the wastegate, the blow-off valve, and positive-displacement and
centrifugal blowers. Also anti-lag and nitrous.

The boost section's Model (`boost.model`): **Machine** is the model below. **Reference** is the original simulator's
simpler turbo, which produces boost from a behavioural rule; it is kept so the two can be compared.

## What is simulated

- **The compressor map from the wheel's sizes** (inducer, exducer, housing A/R, peak efficiency): speed lines, the
  surge line, the choke flow and efficiency islands. A 60 mm wheel at 150,000 rpm makes a pressure ratio of 2.6 to 3.0,
  in line with published maps of that class.
- **Surge as real flow dynamics**, not a scripted effect: the air in the compressor's duct has mass, so when the
  throttle shuts under boost without a blow-off valve the flow reverses and oscillates (about 26 Hz in the test).
- **The turbine** as a nozzle and a wheel, with the textbook efficiency against blade speed. Twin-scroll housings get
  more power from the same mean flow by keeping the pulses apart. Variable vanes.
- **The shaft**: rotor inertia, journal or ball bearings, an optional electric assist motor. Spool-up comes out of
  this, not from a set delay.
- **The charge path**: filter loss, charge pipes, an intercooler with a core that warms and cools, a throttle.
- **The wastegate** (internal or external) with a spring and a controller that holds the ECU's boost target. A
  blow-off valve (to the air or recirculating).
- **Superchargers**: Roots (no internal compression) and twin-screw (internal compression), with leakage, a bypass at
  part throttle, and the power they take from the crank; centrifugal as a compressor geared to the crank.
- **Arrangements**: one, two (a bank each), sequential, compound (two in series), four.
- **Anti-lag**: fuel collecting in the turbine housing and lighting while it is hot. **Nitrous**: extra oxygen in the
  charge at a rated shot, cooling as it evaporates, the bottle emptying.
- **Sounds**: compressor whistle and blade passing, intake rush, blow-off and purge to the air, supercharger whine,
  anti-lag bangs.

Measured on a 2.0 L four at full load on the engine dyno: the turbo matched by the program holds its 70 kPa spring
from 3500 rpm with the shaft at about 105,000 rpm, giving a BMEP of 17.3 bar against 8.9 without boost (production 2 L
turbo fours: 15 to 25). A Roots blower on a V8 at 4500 rpm gives 29 kPa and takes 7.0 N m from the crank.

## What is left out

- A paper filter's flow-dependent loss (one loss coefficient).
- Heat soak of the turbo housing into the compressor.
- Many shape constants of the map are judgement, chosen to look like published maps; the turbine is a single
  efficiency curve.

## What the numbers mean

| Setting or reading | Meaning | Typical |
|---|---|---|
| Boost target (`boost.target`), Gate spring | The pressure the controller aims for; the gate's own minimum | 0.5 to 1.5 bar on road petrol engines |
| Inducer / Exducer diameter, Compressor A/R | The compressor's size; 0 is matched to the engine | Small turbos 40 to 55 mm |
| Turbine wheel, Turbine A/R | Small A/R spools sooner but chokes the exhaust at high rpm | |
| Shaft speed | Thousands of rpm, against a tip-speed limit | 100,000 to 200,000 rpm |
| Pressure ratio, corrected flow, efficiency | The operating point on the map | Efficiency 0.65 to 0.78 in the middle of the map |
| Drive ratio | Pressure before the turbine over boost | 1 to 2 |
| Intercooler effectiveness | Share of the temperature rise it takes out | 0.6 to 0.85 |
| Pulley ratio, Displacement a turn | A blower's speed and size | |
| Blow-off opens at | The pressure drop that opens it | |

## Known gaps today

- **The turbine is fed by a stand-in.** Until the exhaust builder is connected (see [Exhaust](exhaust.md)), the
  cylinders' exhaust gas is fed straight into the turbine housings rather than through the modelled pipes.
- The boost machine costs about twice its share of the processor; it is being made cheaper.
- With no turbo or blower in the path, the charge temperature reads 0 K in some readouts (AIR treats it as missing).
