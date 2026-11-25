# EXHAUST

The exhaust system as built: headers, collectors, turbines, converters, crossover, resonators, mufflers, the bypass
valve and the tailpipes, drawn from the build's settings and the engine's layout.

**Read this first.** The drawing follows the exhaust builder's description of the system (Exhaust model
`exhaust.model` set to **Builder**). The running engine does not use the builder yet: it still runs the original
simulator's exhaust (the **Reference** model, kept so the two can be compared), shaped by the four dials Headers,
Pipe, Crossover and Muffler. So today the builder's settings change the drawing and are saved with the build, but
not the sound or the power. See [Exhaust](../models/exhaust.md).

## What you see

- **Heads and primaries.** One head per bank, ports numbered. Each primary is drawn to scale with the others, so
  unequal-length headers look unequal. The primaries bend into their collectors as plumbed: a 4-2-1 pairs cylinders
  as built, 180 degree headers cross between the banks.
- **Collectors.** A cone for each collector; a log manifold is a chamber the primaries run straight into; a second
  merge (4-2-1) is a smaller cone.
- **Turbine housings** at the head of each side, with downpipes.
- **Each side's chain**, in columns, so a crossover lies straight across: converter (fine cells, or open ones for a
  high-flow converter), main pipe, an H or X crossover, the resonator (a quarter-wave stub, a Helmholtz chamber on its
  neck, or a packed bottle), the muffler (each kind its own shape: a round can with a perforated core, a box with
  baffles, a three-pass box), the bypass valve drawn open or shut with a word, and the tail with one or two tips.
- **Open stacks** end in a bell with radiating arcs and the words "OPEN STACK".
- **Radiating ends** are marked with three arcs, a position tag (REAR LEFT, SIDE RIGHT and so on) and the end's level
  as a bar and in dB.
- **Heat.** Walls are tinted by temperature, from cold blue through to orange and red at 1100 C, with the scale in
  the header. Gas temperatures are written where there is a number. Without wall temperatures from the simulator the
  pipes are outlines and the header says "NO WALL TEMPERATURES YET".
- **Pulses.** Pressure rides along the pipes as a trace, offset across the pipe by pressure (plus or minus 40 kPa is
  the full swing), so you can watch a blowdown pulse run down a primary, meet the collector and come back from the
  open end. Cylinder 1's path from port to tail is the bright trace. **SLOW** replays the last 48 copies at an eighth
  of their pace.

On a machine with a graphics card, the headers and turbines glow with their heat and a flame shows at the tails on
the overrun. These are decoration only: the same facts are in the numbers and tints.

Typical values: exhaust gas at the port of a petrol engine at full load is about 900 to 1100 K (630 to 830 C); at
idle it is far cooler.

## What you can change

Click an element; the inspector shows its own settings.

| Click | Settings |
|---|---|
| A primary | Primary joins (`exhaust.primaryTo`), Primary length, Primary bore, Primary differences, Step at, Step; the Headers dial |
| A collector | Collector joins, Collector pipe length, Collector pipe bore, Collector volume (a log manifold) |
| A turbine | The turbo's arrangement, sizes and gate; Wastegate pipe length and bore |
| Converter | Converter (None, Standard, High-flow) |
| Main pipe | Sides, Main pipe length, Main pipe bore; the Pipe dial |
| Crossover | Crossover kind (None, H, X), Crossover at, Crossover bore, H pipe length, Pipe after crossover |
| Resonator | Resonator (None, Quarter-wave, Helmholtz, Packed bottle), Resonator note, bore, Chamber volume, Bottle length |
| Muffler | Muffler kind (None, Straight-through, Chambered, Three-pass), Muffler size, Muffler chamber |
| Bypass valve | Bypass valve (None, Shut, Open, By speed and load, By the ECU), Bypass bore, Valve opens above, and above load, Valve travel |
| Tail | Tailpipe length, Tailpipe bore, Tips, Exits |
| A head | The engine's layout and the Exhaust model |

A size left at 0 means "the layout's": the builder sizes it from the engine. The selected element has a coloured ring
and its name on a tag.

## Things to try

1. **Watch the pulses.** Hold any build at about 3000 rpm on the dyno and press SLOW. Follow a pulse from cylinder 1's
   port down the primary, into the collector, and the reflection coming back from the open end.
2. **Unequal headers.** Load "Boxer four turbo with unequal headers" and compare the primary lengths on each bank with
   those of a straight four such as "2.0 L turbo four, direct injection".
3. **The dials that act today.** On "Small-block 5.7 L, carburettor-like", change the Headers (`exhaust.headers`) and
   Muffler (`exhaust.muffler`) dials. The drawn primaries change length, and these dials also change what the running
   engine does: listen from the exhaust camera before and after.
4. **Heat.** Hold full throttle on the dyno for 10 to 15 seconds and watch the port gas temperature settle (the
   simulator takes about a second to warm it). Compare with idle.
5. **Overrun and bangs.** On a build with the Maps ECU, the default Bangs (Burned in the pipe) are silent on a fuel-cut
   limiter and a stock lift-off. Set the limiter to Spark cut on TUNE (or Overrun fuel to 0.25), rev to the limiter
   and lift off: listen for the bangs, and look for the flame at the tails on a graphics card. Then set Bangs to Carved
   on EXHAUST and repeat. On the reference ECU, the Pops dial on TUNE fires the carved pops either way.

## A two-stroke's pipe

With a two-stroke engine (Cycle set to Two-stroke, `engine.cycle`) the stage draws each cylinder's expansion chamber
instead of a four-stroke's system: header, widening diffuser cone, belly, narrowing baffle cone and stinger, all to
one scale so a cone looks as steep as it is, with a packed silencer when one is fitted. The pulse and the wave it sends
back ride cylinder 1's pipe. Under the pipe, a plot of the exhaust port's pressure over one turn marks where the port
OPENS and CLOSES: the baffle's returning pulse should arrive just before it closes, pushing back the fresh charge that
escaped. **ON THE PIPE NEAR n rpm** is the speed at which that happens, worked out from the pipe's length and the gas's
temperature: the middle of the power band. Lengthen the pipe and the band moves down; shorten it and it moves up.
Clicking a section of the pipe selects its lengths and bores.

Each tailpipe's end also shows how loud it is, in dB, with a bar under the loudest.

## What is not there yet

- The running engine does not use the builder's settings yet (above). The crossover, resonator, muffler kind, bypass
  valve and converter you choose are drawn but not heard.
- The simulator reports one exhaust gas temperature (at the ports). Wall temperatures, the collector's and tail's
  gas, each end's level and the bypass valve's position are dashes or plain outlines until the builder is connected.
- Only cylinder 1's path carries a pulse trace from the running engine; the fainter traces on the other pipes need
  the builder's per-pipe pressures.
- SLOW replays copies taken at screen rate; truly slowed waves would need the simulator itself slowed.
- The port gas runs hotter than usual today (1150 to 1330 K at full load where 900 to 1100 K is usual), so the tints
  read hot.
