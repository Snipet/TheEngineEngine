# L: the listener, the cameras, the cabin, the places

Branch `claude/awesome-gates-ntdb58`, integrated. `spec.sound.model` 0 (default) is the reference's path, untouched;
1 is package L's. Model and every number's source: [docs/models/listen.md](../models/listen.md).

**Done** (`tee-tests-listen`, `ctest -R '^listen\.'`, 10 tests, 126 s; rig `tests/sim/listen/rig.fm`):
- `Doppler: a pass by trackside...`: 30 m/s past at 8.9 m, heard 1.092 to 0.921 of the pitch, within 0.015 % of
  `c / (c - v cos theta_e)` (retarded time, exact) over the whole pass.
- `the ground: ...`: first and third notches within 2 % of `c / (2 dr)` at 1.5 and 4 m (623, 1039 Hz), 13-16 dB deep.
- `the level falls 6 dB a doubling`: -5.1 to -6.0 dB a doubling, 2 to 30 m.
- `the reference's five views are presets...`: exhaust within 1.1 dB in every view, others within 6.6, 2.86 dB rms
  over 45 pairs; the cabin's boom at 100 Hz within 1 dB.
- `the trackside and chase cameras...`: each source against the exhaust within 8 dB of REAR's / SIDE's; `package M's
  driveline...`: +20.8 dB (against the bay) behind the car, -5.5 over it.
- `no click...`: every camera, place and distance changed, largest step over local rms 0.017 (steady 0.012).
- In the engine: `loudness` (16 piston layouts at full load: exhaust spread 11.3 dB computed, 17.1 by the reference's
  tables), `soak` (40 s driven, 40 listener changes, up to 26.5 m/s: finite, peak 0.897, Doppler 0.98-1.10), `the
  cabin` (highs' share -30.8 dB shut, -18.6 open). Null test `[null]` identical to the bit before every push.

**Cost** (`tee-bench --engine`, V8, 3 runs, busy machine): model 0 1.045 R, model 1 1.112-1.131 R by camera: +0.07 to
0.09 R over the reference's listening path (budget 0.15). Profile: about 7 ns a tap, 25 ns a source (3 taps), `Scene`
150-200 ns, `CabinBoom` 14, `Place` 25.
**Spec** (`spec.sound`, after `mechanical`): `model` (Reference, Places), `camera` (Tailpipe, Rear, Side, Engine bay,
Driver, Trackside, Chase), `place` (Hall, Field, Street, Tunnel, Garage), `enginePlace` (Front, Mid, Rear), `windows`
(fraction), `ground` (Asphalt, Grass, None). DISTANCE moves the camera; VIEW is not read in model 1.
**tel.sound** (after `rate`): `camera`, `range` (m), `doppler` (ratio), `roadPosition` (m, trackside), `loudness` (dB),
`exhaustLevel`, `mixLevel` (dB, 0.4 s means). Builds: `content/builds/listeners/` (V8 tailpipe, trackside, driver, chase).

**Outside my paths**: `sim/engine.fm`: only my calls' arguments (`Listen::block`: `&spec.sound, &tel.sound,
car.speedNow(), set.drive != 0, vTotal, lay.n`, a custom engine's `shake.fx/fy/mx/my`, `lay.fire, lay.cyc,
lay.rotary`; `Listen::mix`: M's `vo.gearbox + vo.axle`). `sim/README.md`: Listen's entry. Wanted: (1) D: `shake.update`
for every engine, not only custom (the boom then has the free forces for the reference's layouts too); (2) G, at its
integration: each radiating end's signal and tag to Listen (one source each: today the two channels); (3) lead:
`spec/telemetry.fm` should include `limits.fm` itself (I include it first).
**Not done**: each turbo and its valves, each tyre as a place of its own (in the bay's, the wheelbase's line); a replay
from another camera (the host's). **FMinus**: no issue met. Package Q's resampler untouched.

**Listen to** (nobody has): the V8 past trackside on the field (the fall in pitch, the comb sweeping); every camera in
turn while revving; the driver's seat, windows shut and open, lugging at 1200 rpm; the chase in the tunnel; DISTANCE
swept at the tailpipe; a single, a flat-plane four and a V12 at full load one after another (one loudness?).
