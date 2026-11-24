# Card F: forced induction

*Wave 1. Repository: TheEngineEngine, integration branch `claude/awesome-gates-ntdb58`. Read
[01-rules.md](01-rules.md), [02-contracts.md](02-contracts.md), `sim/README.md` (K3's: the `Boost`, `Intake` and
`Exhaust` seams), then the reference: `Extras::turbo16` and the turbo's and supercharger's sounds in
`sim/ref/sounds.fm`, and in `sim/ref/engine.fm` the turbine's part of `pipes()` (the narrowed collector pipe and
its low-pass), `pBox`, `tAir` and the supercharger's torque in `sampleTick`.*

**OWNS**: `sim/boost/**`, `sim/spec/boost.fm`, the `tel.boost` section and its Scope arrays,
`tests/sim/boost/**`, `content/builds/boost/**`, `docs/models/boost.md`.

**Budget**: 0.12 R a sample with two turbos working; 0 when `boost.kind` is none.

**Start standalone.** Until K3's split lands you can build and prove everything below against a made-up engine
(an air pump: `mdot = VE * rho * Vd * N / 120`; an exhaust that gives that mass back hot, steady or in pulses) in
a test harness. Integration is the last third of the work, not the first.

## What the reference does

A turbo is a number, `spool`, that chases the square root of the exhaust's power with a lag; boost is its square
up to a wastegate's clamp; the blow-off valve and the flutter are envelopes; the turbine is a narrower pipe with a
low-pass. It sounds right and is tuned by ear, and it stays, bit for bit, as `boost.model = 0`, the default.
`boost.model = 1` is a machine in the gas path.

## The model

**The charge path**, as volumes that hold mass and energy (filling and emptying, every sample): inlet (the air
outside, through the filter's loss) → compressor → the charge pipe (with the blow-off valve) → the intercooler
(a pressure drop that grows with the flow squared, an effectiveness, a thermal mass that soaks) → the volume
before the throttle. `Intake` reads this last volume's pressure and temperature where it read `pBox` and `tAir`.

**The compressor.** A map made from a few sizes, not a table of someone's product: speed lines as a fitted
family (for a shaft speed: the pressure ratio flat toward surge, falling as an ellipse to choke; the choke flow
from the inducer's area, the pressure ratio at the knee from the exducer's tip speed), an efficiency island about
a best flow and tip Mach number, and a branch for reversed flow. Inputs a person knows: inducer and exducer
diameters (so, the trim), the housing's A/R as a width of the map, the peak efficiency. The outlet's temperature
from the ratio and the efficiency; the power from the mass flow and the temperature rise. **Surge is not an
effect**: the flow through the compressor has the inertia of the air in its pipe (`d mdot/dt = (A/L)(p_in *
ratio(mdot, N) - p_volume)`), and with the map's shape left of its peak that system oscillates by itself when
the throttle shuts (Greitzer's model). That oscillation, radiated from the inlet, is the flutter.

**The turbine.** A nozzle: flow from the upstream state, its effective area (the wheel and the A/R; a variable
nozzle's vanes scale it) and the pressure ratio by the same compressible law the valves use (`pipes.fm`'s tables),
choked when it chokes; an efficiency that peaks at a blade speed ratio near 0.7 and falls away either side; power
from the flow, the inlet temperature, the expansion and the efficiency. **It sits in the pipes**: card G's
collector pipes end in your turbine housing, a small volume with mass and energy; the nozzle empties it into G's
downpipe; the wastegate is a second orifice from the same volume, to the downpipe (internal) or to a short open
pipe of its own (external), which G radiates. A twin-scroll housing is two volumes fed by two groups of cylinders
into one wheel. Because the volume's pressure rises with each blowdown and the nozzle is solved every sample, a
pulse does more work than its mean would: the reason for a twin-scroll housing is then in the model, not added.
The seam with G, to agree with its agent and write into `sim/README.md`: G gives you, for each pipe end at a
housing, the arriving wave and the pipe's impedance and gas temperature; you give back the housing's pressure (G
solves its own ends against it, as the reference solves the runners against the plenum) and, for the downpipe,
the flow and temperature you put into it. Everything at this junction is passive: test it (below).

**The shaft.** `J dw/dt = (P_turbine - P_compressor) / w - friction`; `J` from the wheels' sizes or given;
journal or ball bearings as a friction choice; an optional motor's torque (electric assist) with a power limit.

**Control.** The wastegate's actuator: a spring that opens it at a boost pressure by itself, and a duty from the
ECU (card H gives a boost target; a PI loop here or there: agree, and say which in `sim/README.md`) that holds it
shut longer. Vanes by an ECU map. Over-speed and over-boost flagged.

**The blow-off valve**: none, to the air, or recirculating; opens on the pressure across the throttle against its
spring. With none, the surge above is what happens.

**Arrangements** (`boost.layout`): one; two side by side, a bank each; two in sequence (the second joins above
a flow); two in series (compound: the first stage feeds the second); four as two a bank. At most `MAX_TURBOS`
shafts are simulated.

**Superchargers** (`boost.kind`): Roots (a volume a turn, leakage back, no internal compression: the outlet's air
is compressed by backflow, so it runs hot and its pulses are sharp); twin-screw (a built-in volume ratio: cooler,
its pulse softer when the ratio matches the boost); centrifugal (the compressor above on a step-up gear and a
pulley: boost with the square of the revs). The pulley's ratio; a bypass at part throttle; the torque it takes
from the crank from the air's work over its efficiency, given to `Mechanics`.

**Anti-lag** (with card H): the ECU retards the spark, adds fuel and lets air past the throttle; here, that fuel
burns in the housing, as events with their own randomness: heat into the volume, so the wheel keeps its speed off
the throttle, and bangs down the pipe.

**Nitrous**: a mass flow by the shot's size while its window is open (the ECU's), from a bottle whose pressure
falls with use; its oxygen raises the charge's oxygen share (a number `Combustion` reads; 0.232 for air) and its
evaporation cools the charge. A purge is a source.

## Sound sources

Signals a sample, each from the model's own state (the rule of the reference): the compressor's rush (with flow
and tip speed); a whistle at the shaft's own frequency and, where it is below 20 kHz, the blades' passing; the
surge's pulses at the inlet; the blow-off valve's jet; an external gate's pipe (G radiates it); a supercharger's
whine from its rotors' real speed and the pressure it meets, as the reference builds it; the purge.

## Telemetry and Scope

Each shaft's speed; the compressor's pressure ratio, corrected flow, efficiency and outlet temperature; the
intercooler's outlet temperature; the gate's and the vanes' positions; the pressure and temperature before the
turbine and the ratio of that pressure to the boost; surge, choke, over-speed flags; a supercharger's power.
Scope: the last few hundred points of (corrected flow, pressure ratio), and the map's own lines, surge line and
efficiency contours as arrays, so a view can draw the map and the trail without knowing the model.

## Tests

Standalone:
1. The map: flow at choke scales with the inducer's area; the knee's ratio with tip speed squared; efficiency
   peaks where it should.
2. A bigger turbine housing spools later and chokes later; a bigger compressor on the same turbine lags more;
   time to a target boost at a fixed engine speed grows with `J`. Monotonic, each.
3. In steady state the turbine's power equals the compressor's plus friction; mass in equals mass out.
4. The gate holds a target within 5 % across speeds where the turbo can make it; the spring alone holds its own
   pressure.
5. A shut throttle on boost with no blow-off valve: the flow reverses and oscillates between 5 and 40 Hz and dies
   away; with a valve it does not reverse.
6. Pulsed exhaust into a twin-scroll housing gives more shaft power at low speed than the same mean flow into a
   single one.
7. Superchargers: a Roots's boost is nearly flat with speed and its outlet hotter than a twin-screw's at the same
   boost; a centrifugal's rises with speed squared; the crank's torque taken matches the air's work.
8. The intercooler's outlet temperature against its effectiveness; nitrous's torque step within 15 % of its
   rating on the made-up engine.
9. Passivity: the housing with the wheel locked and no fuel never returns more wave energy than it took, for
   random sizes. Ten minutes of random throttle chops: finite, bounded.

Integrated (K3's `EnginePlayer`, `tee-dyno` when card C has it): `model = 0` passes the null test; a 2 L turbo four
makes a boost and a mean effective pressure a real one makes (state the ranges and their source); the pressure
before the turbine is between 1 and 2 times the boost at full power; shaft speeds between 80,000 and 250,000 rpm
by size; the bench within budget.

## Not in this card

The ECU's maps and the anti-lag's and nitrous's switching logic (card H). The pipes and what radiates (card G).
How any of it is heard from a place (wave 2). Twin-charging a turbo with a supercharger (later).
