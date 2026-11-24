# M: mechanical voices

Branch `claude/awesome-gates-ntdb58`. `spec.voices` (`sim/spec/voices.fm`, appended to Spec), `sim/sound/` (`Voices`
and seven voice modules beside `Extras`), model `docs/models/voices.md`, tests `tee-tests-sound` (ctest `sound.*`).
`spec.voices.model` 0 (default) is the reference to the bit: `[null][quick]` identical before every push.

**Done** (`tee-tests-sound`, 24 cases, all pass, ~40 s):
- `whine:` constant mesh, each gear's pair, final drive within 0.5 % of their tooth frequencies, gears 1-6 at
  150/300/520 rad/s; reverse's idler; direct gear unloaded; louder with load and speed; straight cut +12.0 dB; sidebands;
  silent on the bench and at rest. `rattle:` dog box in neutral from the crank's own ripple; none with the clutch out,
  in gear, a smooth crank, an automatic; manual quieter; fades with revs.
- `drives:` alternator 100.8x, water 7.2x, steering 11x; cam belt 21x, cam gears 30x (+60x), louder with revs;
  supercharger gears 40x, louder with boost. `fan:` electric on/off with 5 K hysteresis, its blades' line, coasts down,
  off with the key; viscous roar monotonic with coolant and revs.
- `pump:` prime at key-on for `prime` s, lines at 41 and 8 x 95 Hz, runs on while the crank turns. `injectors:` DI tick
  per injection; none for port or diesel. `lifters:` solid tick monotonic in lash and speed. `float:` two bounces a
  landing with E's clatter. `wastegate:` chatter only part open, with flow; external dump to air monotonic with flow.
  `starter:` 130x ring mesh, 20 segments at 13x / 47x, louder with power, solenoid and pinion strike, run-down.
- `clicks:` no voice steps on or off. `soak:` 10 min random, finite, peak < 1.
- `engine:` (whole simulator, Scenario): bench whine on/off identical to the bit; alternator line on the bench; constant
  mesh at 19 x crank in third (706.4 Hz found at 706.4); starter kinds differ while cranking.

**Measured**: cost all on 0.06-0.07 R (V8 drive-car 1.092 vs 1.021 off; free-revs 1.087 vs 1.027; budget 0.08).
Level (view Engine, drive-car): voices 6-18 dB under the reference's engine bay, which is ~35 dB under the mix.
Compile: no measurable change (2.3-2.4 s machine code).

**Integration**: `Extras` gained `control16(...)`, `busy()`, `valveClick(amp, intake)`, `setup(..., &spec.voices)`.
I added the `ex.control16` call (last in `Engine::control16`, skipped at model 0) after twenty minutes without an answer
from card D, and wired `vt.clatter` into it after E's merge. Each voice has its own output for card L: `ex.vo.gearbox`,
`.axle`, `.front`, `.head`, `.tank`, `.gateOut`, `.starter`; their sum is in `ex.mech` meanwhile.

**Since the handoff**: card D's key-on rendering landed: `engine: the fuel pump's prime is heard at key-on` passes
(the prime before the first crank; the reference renders nothing there). Card F's gate wired: with `boost.model` 1 the
call passes the most open of `mach.telGate[]` and `spec.boost.gate`; `engine: card F's wastegate` (turbo-four, dyno at
5500 rpm full load): no chatter while idling with the gate shut, chatter 1.5e-5 rms at full load, an external gate to
the air 2.4e-4. README rows added. Listening scenarios: `tests/sim/sound/listen/` (ten files and a README for X).

**Wanted outside my paths**: card L, place the seven outputs (gearbox, differential, front of engine, head, tank,
turbo, flywheel end). Card B2: the chain's lash as an event, for the bike voices.

**FMinus**: no bug. A pointer cannot be compared with 0 (`out != 0` refused): the rig always writes.

**Listen to** (view Engine, `spec.voices.model` 1): V8 drive-car in third at part throttle (whine, 706 Hz at 2230 rpm);
same with `gearCut` Straight cut; `car.model` 1 Sequential idling in neutral, clutch up (dog rattle), then press the
clutch; bench at 3000 rpm with `timing` Gears, then Toothed belt; `lifters` Solid, `lash` 0.0005, idle; `injection`
Direct at idle; `fanOn` 300 (electric fan running), then `fan` Viscous through the revs; a start with `starter` Direct
drive then Gear reduction; a turbo build at full boost with `gateDump` To the air. Nobody here has heard any of it.
