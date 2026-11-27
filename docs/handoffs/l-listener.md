# L: the listener, the cameras, the cabin, the places

Branch `claude/awesome-gates-ntdb58`, integrated. `spec.sound.model` 0 (default) is the reference's path, untouched;
1 is package L's. Model and every number's source: [docs/models/listen.md](../models/listen.md).

**Done** (`tee-tests-listen`, `ctest -R '^listen\.'`, 15 tests, 3.5 min; rig `tests/sim/listen/rig.fm`):
- `Doppler: a pass by trackside...`: 30 m/s past at 8.9 m, heard 1.092 to 0.921 of the pitch, within 0.015 % of
  `c / (c - v cos theta_e)` (retarded time, exact) over the whole pass.
- `the ground: ...`: first and third notches within 2 % of `c / (2 dr)` at 1.5 and 4 m (623, 1039 Hz), 13-16 dB deep.
- `the level falls 6 dB a doubling`: -5.1 to -6.0 dB a doubling, 2 to 30 m.
- `the reference's five views are presets...`: exhaust within 1.1 dB in every view, others within 6.6, 2.86 dB rms
  over 45 pairs; the cabin's boom at 100 Hz within 1 dB.
- `the trackside and chase cameras...`: within 8 dB of REAR's / SIDE's balance; `package M's driveline...`: +20.8 dB
  behind the car, -5.5 over the bay. M's gearbox, axle, tank, front, head, gate each at its place. `the free camera...`
  (camera 7 at SIDE's place within 0.3 dB; the chase 7.5 m back at 30 m/s); `the rider's seat` (B2's bikes).
- `no click...`: every camera, place and distance changed, largest step over local rms 0.017 (steady 0.012).
- Headroom (X-004): `the last stage` (sources 60 dB too loud: peak 0.70), `the ceiling` (LEVEL +12, 0.5 m: 0.733).
  Auto level (lead 20:13, an exposure, on by default): `the level at the defaults` (first, kei, V12, tractor: free
  rev typical 0.18-0.42, largest 0.30-0.53, idle 0.10-0.13), `the auto level` (Driver 3.4 dB under Rear off, 0.3 on).
- In the engine: `loudness` (16 piston layouts at full load: exhaust spread 11.3 dB computed, 17.1 by the reference's
  tables), `soak` (40 s driven, 40 listener changes, up to 26.5 m/s: finite, peak 0.718, Doppler 0.98-1.10), `the
  cabin` (highs' share -30.8 dB shut, -18.6 open). Null test `[null]` identical to the bit before every push.

**Cost** (`tee-bench --engine`, V8, 3 runs, busy machine): model 0 1.045 R, model 1 1.112-1.131 R by camera: +0.07 to
0.09 R over the reference's listening path (budget 0.15). Profile: about 7 ns a tap, 25 ns a source (3 taps), `Scene`
150-200 ns, `CabinBoom` 14, `Place` 25. **Spec** (`spec.sound`, after `mechanical`): `model` (Reference, Places), `camera` (Tailpipe, Rear, Side, Engine bay,
Driver, Trackside, Chase), `place` (Hall, Field, Street, Tunnel, Garage), `enginePlace` (Front, Mid, Rear), `windows`
(fraction), `ground` (Asphalt, Grass, None), `listenerX/Y/Z`, `facing`, `autoLevel` (True levels, Auto). DISTANCE moves the camera; VIEW is not read in model 1.
**tel.sound** (after `rate`): `camera`, `range` (m), `doppler` (ratio), `roadPosition` (m, trackside), `loudness` (dB),
`exhaustLevel`, `mixLevel` (dB, 0.4 s means), `limiter` (dB), `ceilingParts`, `autoGain` (dB). Builds: `content/builds/listeners/` (V8 tailpipe, trackside, driver, chase).

**Outside my paths**: `sim/engine.fm`: only my calls' arguments (`Listen::block`: `&spec.sound, &tel.sound,
car.speedNow(), set.drive != 0, vTotal, lay.n`, a custom engine's `shake.fx/fy/mx/my`, `lay.fire, lay.cyc,
lay.rotary`; `Listen::mix`: M's seven voices, `vo.all`, `ex.mouth`). `sim/README.md`: Listen's entry. Wanted: (1) D: `shake.update`
for every engine, not only custom (the boom then has the free forces for the reference's layouts too); (2) G, at its
integration: each radiating end's signal and tag to Listen (one source each: today the two channels); (3) lead:
`spec/telemetry.fm` should include `limits.fm` itself. **Open**: Trackside at a standing idle, 250 Hz 14 dB over the other cameras. **Not done**: each turbo, each tyre as its own place; a rider
a replay (the host's). Scenarios for the pack: tests/sim/listen/scenarios/ (16). **FMinus**: no issue met.
**Listen to**: listen.md's three marked first; the V8 past trackside, every camera revving, the cabin, a single, a V12.
**26 Nov, X-049 (1)**: the cut is not starved. Model 0 sits on its 0.85 limiter all pull (0.3 m Exhaust view), so its cut's 0.36 is a dip from a capped ceiling and no relight can stand out; model 1 (auto -14.5 dB) dips 9.4 dB under the firing (small four 8.6; model 0 there 10.7). The turbo six's relight +10 dB over the firing (small four +0.6) is N1's (2); the ratio is the judge's (X).
