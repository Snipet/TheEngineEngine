# Exhaust

The pipes from the exhaust ports to the open air, in which pressure waves travel, reflect and add up: what makes a
system scavenge well at one rpm, and the pressure waves that leave the tailpipes for the listener.

Exhaust model (`exhaust.model`): **Reference** is the original simulator's exhaust, shaped by four dials (Headers,
Pipe, Crossover, Muffler) and three fixed arrangements. **Builder** builds any system from a description, out of the
same pipe and junction parts. Reference is kept so the two can be compared.

**Today the running engine uses the Reference exhaust in every build.** The builder is finished and tested on its
own, and its settings are kept in each build and drawn on EXHAUST, but the engine does not run them yet.

## What is simulated

Both models:

- **Pipes as wave guides**: a pressure pulse travels down a pipe at the speed of sound in hot gas, loses a little to
  the walls, and partly reflects where the pipe changes area, branches or ends. A strong pulse steepens as it goes.
- **Junctions** where pipes meet, with the textbook losses for sudden expansions and contractions.
- **Open ends** that reflect inverted and radiate the sound; each end carries a position (rear left, side right and so
  on) for the listener.
- **Wall heat**: the gas cools along the pipes and the walls warm and soak.
- **Pops and bangs**: unburned fuel lighting in the collector. On the builder, **Bangs** (EXHAUST) chooses how:
  **Burned in the pipe** (the default) releases the fuel's heat in the collector's gas over its own burn, a few
  milliseconds, and the pipes make the sound. Only fuel that left the cylinder still mixed with its own air bangs (a
  spark cut, a misfire, a fuelled overrun's late burn, anti-lag), so a stock car's fuel-cut limiter and lift-off make
  none. **Carved** is the reference's pulse and crackle. The reference ECU's Pops dial fires the carved pulse either
  way.

The builder adds, from settings:

- Headers: which collector each primary joins, each primary's length and bore, stepped primaries, 4-2-1 and tri-Y
  merges, log manifolds, open stacks ("zoomies").
- One or two sides; converters (standard, high-flow, none); an H or X crossover at the head or after the main pipes;
  resonators (quarter-wave stub, Helmholtz, packed bottle); mufflers (straight-through, chambered, three-pass, none);
  a bypass valve round the muffler (shut, open, by speed and load, by the ECU); one or two tips.

Measured on the builder with made-up pulses: 180 degree headers cut a crossplane V8's off-firing-order content by 24
to 27 dB; a quarter-wave resonator notches its note by 38.5 dB; a chambered muffler takes 22.2 dB out, a three-pass
15.9, a straight-through 1.8; an open bypass valve adds 16.7 dB. Every random system tested is stable and conserves
flow.

## What is left out

- The two sides' gas being different; directivity of the open ends (the listener handles placement).
- Bends, except as a small loss per pass.
- Two-stroke expansion chambers are a separate model (see [Two-stroke](twostroke.md)).

## What the numbers mean

| Setting or reading | Meaning | Typical |
|---|---|---|
| Headers, Pipe, Crossover, Muffler (dials) | The Reference exhaust's lengths, crossover share and muffler size: these act today | |
| Primary length, Primary bore | Longer primaries tune lower; 0 is the layout's | 0.6 to 1.0 m on road headers |
| Collector joins, Collector volume | How primaries merge; a volume makes a log manifold | |
| Crossover kind (None, H, X) | Joins the two sides | |
| Resonator note | The frequency a resonator cancels | |
| Muffler kind, Muffler size | How much is taken out and how much back pressure is added | |
| Exhaust gas (reading) | Gas temperature at the ports | 900 to 1100 K at full load petrol; diesel 700 to 1000 K |

## Known gaps today

- **The exhaust builder's settings are kept but not yet run by the engine.** Crossover kind, resonators, muffler kind,
  converter and bypass valve are not heard; the four dials are.
- **Port gas runs hot**: 1150 to 1330 K at full load where 900 to 1100 K is usual. The cylinder's heat balance is
  being checked; tints on EXHAUST read hot.
- Wall temperatures, per-pipe pressures and each end's level are not reported yet, so EXHAUST shows outlines and
  only cylinder 1's pulse.
- A full dual system with crossover, resonators and valves costs more processor time than planned (about 0.3 of the
  original engine's whole cost).
- Turbines are not yet connected through the modelled pipes (see [Forced induction](boost.md)).
