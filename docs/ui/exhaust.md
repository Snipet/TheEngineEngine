# EXHAUST

*Package U3. Code: `ui/views/exhaust/`. Tests: `tests/ui/views/ExhaustTests.cpp` (`ui.exhaust.*`).*

## What the stage shows

The exhaust as built, drawn from the same rules the simulator's builder uses to make it (`sim/exhaust/slots.fm`
`resolve()`, `shape()`; the layouts of `sim/core/layouts.fm`). `ExhaustModel.cpp` mirrors them in the same order.

- **Heads and primaries.** One head a bank (the left bank above), each port numbered. Each primary's straight run is
  to scale with the others, as built: the reference's 6 % spread, a boxer's longer second bank, the lengths a build
  sets. So unequal-length headers look unequal. The primaries then bend into their collector **as plumbed**: a 4-2-1
  pairs 1 with 4, and 180-degree headers cross between the banks.
- **Collectors.** A collector is a cone (an area strip). A log manifold (`colVol > 0`) is a chamber the primaries run
  straight into. A second merge (4-2-1) is a smaller cone. Collectors that would overlap are spread apart.
- **Turbine housings** sit at each side's head (card F's arrangement: one, a bank each, sequential, compound, four),
  with downpipes to the sides.
- **Each side's chain** runs in columns shared by both sides, so a crossover lies straight across:
  - converter (a substrate of fine cells, or open ones for high-flow) and main pipe;
  - an **H** (a pipe between the lanes) or an **X** (the lanes meet at one node and part), at the head or after the
    main pipes;
  - resonator: a quarter-wave stub, a Helmholtz chamber on its neck, or a packed bottle in line;
  - muffler, each kind its own shape: a round can with a perforated core, a box with baffles, a three-pass box;
  - the bypass valve round the muffler, its butterfly drawn along the pipe (open) or across it (shut), with a word;
  - the tail and its one or two tips.
- **Open stacks** end in a bell with radiating arcs and the words "OPEN STACK, FRONT".
- **Radiating ends** are three arcs, the position tag (REAR LEFT, SIDE RIGHT, ...) and the end's level as a bar and
  in dB.
- **Heat.** Every wall is tinted by its section's temperature, cold blue through ink to orange and red up to 1100 C
  (the gas at the ports reaches 1100 to 1330 K at full load); the scale is in the header.
  - The gas temperatures are numbers, each with its label: under the stage, and on the drawing at the port and the
    collector (only where there is a number).
  - Without wall temperatures, the pipes are outlines and the header says "NO WALL TEMPERATURES YET" once. The tinted
    state is pictured with made-up demo telemetry (`exhaust.build.*`, `exhaust.stage`).
- **It fits any stage.** The parts' widths are in a unit that shrinks to 0.55 on the frame's narrowest stage, and the
  primaries stay to scale with each other. It is tested at the frame's stage for a 1100x700 and a 1600x1000 window
  (`ui.exhaust.framestage`: every part inside, room for the ends' tags).
- **Pulses.** The Scope's `pipePressure` is cylinder 1's path from port to tail: 64 points spaced evenly by length.
  It rides that path as a trace, offset across the pipe by pressure (+-40 kPa is the full swing). Blowdown pulses can
  be watched running out, meeting the collector and coming back from the open end.
  - **SLOW** replays the last 48 copies at an eighth of their pace, timed only by `step(dt)`. While it catches up,
    the stage keeps moving.
  - Truly slow motion of the waves would need the simulator itself slowed (copies come at frame rate). That is a
    request in the handoff.

## Atmosphere

U5's shaders (`ui/views/fx/Fx.h`) are drawn behind the primitives, and they carry no fact: the numbers and tints stay
primitives. They are:

- a glow over the headers (from their wall's temperature, else the gas at the ports) and over each turbine housing;
- a flame at each tail while `exhaust.pop` is live, or while `engine.flags` has the fuel cut and the throttle is shut
  (the overrun).

On the software rasteriser they draw nothing, so every golden is the stage without them. `ui.exhaust.effects` checks
that the draw list is identical with the effects on and off, and that the stage at rest stays at rest. Their flicker
runs only while one of them would draw.

## What is clickable

Each element selects a path. Its inspector page (`inspectorPage()`) lists that element's own fields: its port's, its
side's, or its collector's. A field the build lacks is left off; with none, the frame shows its own pages.

| Element | Selection | Fields |
|---|---|---|
| a primary | `exhaust.primary[i]` | `primaryTo[i] primLen[i] primD[i] primSpread primStepAt primStepD`, the HEADERS dial |
| a collector or merge | `exhaust.collector[c]` | `collectorTo[c] colLen[c] colD[c] colVol[c]` |
| a turbine | `boost` | the kind and arrangement, the turbine's sizes and gate, `exhaust.gateLen gateD` |
| converter | `exhaust.converter[s]` | `converter[s]` |
| main pipe | `exhaust.main[s]` | `sides mainLen[s] mainD[s]`, the PIPE dial |
| crossover | `exhaust.crossover` | `crossover crossAt crossD crossLen postLen` |
| resonator | `exhaust.resonator[s]` | `resonator resHz resD resVol resLen [s]`, `linkLen` |
| muffler | `exhaust.muffler[s]` | `muffler mufSize mufLen bypass [s]` |
| bypass valve | `exhaust.bypass[s]` | `bypass[s] bypassD bypassRpm bypassLoad bypassTime` |
| tail | `exhaust.tail[s]` | `tailLen tailD tips exitAt [s]` |
| a head | `engine` | `engine.layout`, `exhaust.model` |

The selected element has a ring in the accent and its name on a tag. The element under the pointer has a faint ring.

## What is derived, and what is judgement

- **Derived, as the simulator does it:** the topology (layout defaults, a collector's side, merges, sides fed by a
  crossover), the primaries' lengths, the tags.
- **Judgement, for the picture only:**
  - the length of each piece of cylinder 1's path used to place the Scope's points: collector pipe 0.38 m (a bike
    0.25), muffler twice its first chamber, tail 0.40 m;
  - where a single turbine sits when the build has two sides.
- **Absent data:** a wall with no temperature is a neutral grey, a number is a dash, and the trace is replaced by the
  words "no pipe pressure from the Scope yet". The demo's absolute pipe pressures are taken over the air outside.
