# AIR

*Package U3. Code: `ui/views/air/`. Tests: `tests/ui/views/AirTests.cpp` (`ui.air.*`).*

## What the stage shows

**The charge path** is a row of parts in the order the build has them. A part the build lacks is not drawn.

- **The parts:**
  - the air box with its filter's pleats (none when individual throttle trumpets open to the air);
  - the compressors: one scroll, two in parallel, sequential (marked PRIMARY and SECONDARY), compound (two in
    series), or four. A supercharger instead: Roots (two lobed rotors), twin-screw (the flights), or centrifugal;
  - the intercooler, with fins;
  - the blow-off valve on the charge pipe;
  - the throttle, a butterfly drawn at its opening with the percentage;
  - the plenum;
  - the runners to the head, or individual throttle bodies, each with its trumpet and its own butterfly.
- **Beneath each part:** its live pressure (absolute) and temperature as numbers. Their units are written once, at the
  left of the rows. When both are absent, a single dash.
- **The pipe between two parts** is tinted by the air leaving the first, from blue near the air outside to orange
  some 190 K above it, with its temperature on the pipe (only where there is one).
- **The row fits any stage:** the parts' widths are in a unit that shrinks to 0.58 on the frame's narrowest stage. A
  name that would touch its neighbour's drops a line.

**The compressor's map** comes from the simulator when it sends one, and otherwise from a copy of its arithmetic:

- The simulator's map is the Scope's `boostLine*`, `boostSurge*` and `boostContour*` arrays, with its trail the
  `boostTrail*` ring. The lines are read again only when `boostMapRev` moves, and the panel then says FROM THE
  SIMULATOR (test `ui.air.scopemap`).
- Otherwise, for a build edited with the engine stopped, the map is made from the machine's sizes with the same
  arithmetic.

- The sizes are matched to the engine where the build leaves them at 0, as `machine.fm` does.
- The lines are `compressor.fm`'s: speed lines from knee to choke at tip Mach 0.4 to 1.45, numbered in thousands of
  rpm; the surge line through the knees (amber, with the word); the choke line; five efficiency islands.
  - The islands are traced over 36 speed steps rather than the simulator's 8, so they are smooth.
  - A left crossing that falls left of the knee is held on the surge line. That side is unstable and no line is
    drawn there.
- The operating point carries a trail of up to 180 moves, fading with age, so a spool, a shift and a surge leave
  their shapes.
- **Two machines: one map with both points** (turbo 1's map, as `mapLines()` makes; turbo 2's point is a ring
  marked 2). For a compound pair the high-pressure stage's own map differs, and only its point is shown.
- A Roots or twin-screw blower has no map, and the stage says so in words.

**Beside the map**, in a column of its own, the operating point's numbers and the shaft:

- pressure ratio, corrected flow and efficiency;
- the shaft's speed in krpm, and a bar against its limit (a tip speed of 560 m/s, or the build's);
- the second turbo's speed;
- for variable vanes, a nozzle ring whose vanes stand more nearly radial as it opens, with the opening.

The map takes the rest of its panel. A press on the column selects the shaft.

Without a map, the left panel shows something else instead:

- a Roots or twin-screw blower: its drive (pulley ratio, bypass, the power it takes);
- a natural engine: the manifold (pressure, vacuum, throttle).

**The gate and the blow-off valve:**

In the right-hand panel, as tall as what it shows:

- poppet valves drawn lifting off their seats, with the opening or lift in words and numbers;
- boost against target as a bar with the target marked, both as numbers;
- a trace of recent boost with the target dotted, only once there is a trace and room for it.

**Anti-lag and nitrous**, where the build has them: a lamp and words (ARMED, ACTIVE or ON) and the bottle's state
("3.69 kg OF 4.50 kg").

## What is clickable

Each part's inspector page lists its fields (those the build has):

| Part | Selection |
|---|---|
| air box | `intake.airbox` |
| a turbo | `boost.turbo[i]` |
| supercharger | `boost.supercharger` |
| intercooler | `boost.intercooler` |
| throttle | `intake.throttle` |
| plenum | `intake.plenum` |
| runners | `intake.runners` |
| the map | `boost.compressor` |
| the shaft | `boost.shaft` |
| the gate | `boost.gate` |
| the blow-off valve | `boost.blowOff` |
| anti-lag | `boost.antiLag` |
| nitrous | `boost.nitrous` |

## What is derived, and what is absent

- **Derived:** the arrangement from `boost.kind` and `boost.layout`, matched compressor sizes, the map, the shaft
  limit, and the target (the ECU's if the telemetry has it, else the build's target, else the spring).
- **Absent:** every live value the simulator does not write is a dash, and the map says "no operating point yet".
  The names are one table, `AirNames` in `AirWorkspace.h`.
  - Card F's `tel.boost.*` is integrated: shaft speed, ratio, flow, efficiency, compressor outlet, gate, vanes,
    drive, charge pipe, intercooler outlet, throttle inlet, blow-off valve, supercharger power and flags.
  - Card E's `intake.boxPressure` and `intake.plenumTemperature` are not integrated yet. Until they are, the air after
    the throttle takes the temperature before it.
  - The bypass's position, anti-lag being active and the bottle's contents are not in the telemetry. The stage says
    what the build has (BYPASS AT PART THROTTLE, ANTI-LAG ARMED, A 4.50 kg BOTTLE), and the nitrous lamp lights from
    flag 32.
- **Demo:** U1's hook (`TEE_UI_DEMO(air, airDemo)`) adds this package's fields and its telemetry, following the
  demo's script (`AirDemo.cpp`).
