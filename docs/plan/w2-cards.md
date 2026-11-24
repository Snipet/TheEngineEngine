# Wave 2: the cards in outline

*Written before wave 1 has run, so deliberately shorter than wave 1's cards: each is expanded into a file of its
own at the gate (checkpoint M2), when the seams it builds on exist and your listening has said what matters most.
The rules ([01-rules.md](01-rules.md)) and the contracts ([02-contracts.md](02-contracts.md)) bind them as they
are. Package Q can start as soon as K3b lands; it does not wait for the gate.*

## Q. Performance (early)

*Owns `docs/measurements/**`, and by pull request to their owners, changes inside any `sim/` module that leave
its output within -80 dB.* Where does R go? Profile the split simulator by module (card C's bench). Then make R
itself smaller: the resampler (96 taps a host sample today); filters whose coefficients are recomputed more often
than they change; the valves' three passes where two converge; work that can move to the 16-sample rate. Try
FMinus's own levers and report each with numbers: the math mode, `param` on the fuel and cycle switches (one
specialised hot loop each), the knob-version cap, -O3. Study, and only then propose: advancing the two banks,
the intake side and the post-collector exhaust in parallel for 16 samples at a time (legal wherever the only
link is a pipe longer than 16 samples). **Target: R down by a fifth with no audible change, and a typical turbo
V8 under 60 % of the user's core.**

## L. The listener, the cameras, the cabin, the places

*Owns `sim/listen/**`, `sim/spec/sound.fm`, `tel.sound`.* Replaces the reference's five gain tables with
00-abstract-plan.md §5. **Sources**: every radiating end of card G, the intake's mouth or trumpets, the block,
each turbo and its valves, the gearbox, the differential, each tyre, the wind: each a signal and a place on the
car (the engine's place, front, mid or rear, is a Spec field). **A listener**: a place and a facing; for each
source a delay line read at a moving fraction (Doppler for free; the reference does this for the tailpipes), the
level with distance, the air's loss of highs, the source's directivity, and a second tap for the ground's
reflection. Two ears as the reference's ORTF pair, or a head's shadow and delay. **Cameras**: tailpipe, trackside
(the car passes: its position from card I), chase, engine bay, driver. **The cabin**: structure-borne sound from
card D's shaking forces through mounts into the reference's cabin modes; the exhaust's drone through the floor;
the firewall's and the glass's filters; windows; wind and tyres by speed. **Places**: field, tunnel, garage,
street: the reference's reverb with early reflections for each. **Level**: one loudness across builds, computed,
where the reference keeps a measured table by layout. *Tests*: Doppler's ratio at a pass against the formula; the
ground comb's notches where the geometry puts them; level against distance; each camera's balance of sources
within stated ranges; `view` 0 to 4 of the reference reproduced within a stated tolerance as presets of the new
model; the null test with the old path selected.

## M. Mechanical voices

*Owns `sim/sound/**`.* The reference's `Extras`, grown, each voice driven by the model's own events and speeds:
gear whine by tooth count, shaft speed and load, straight-cut or helical (card I gives ratios, speeds, torque);
the differential; a belt and a supercharger's drive; the cooling fan and its clutch; the fuel pump's prime at
key-on; port and direct injectors; valvetrain character by kind, and card E's float clatter; a wastegate's
chatter; the starter by size. *Tests*: each voice's line where its order says, its level against load and speed
monotonic, silence when its cause is absent.

## T. Two-strokes

*Owns `sim/core/twostroke.fm`, additions to `sim/spec/engine.fm`, `sim/exhaust/chamber.fm`, its tests and
builds.* A second cylinder model beside the four-stroke one (the cycle is 360 degrees): exhaust and transfer
ports opened by the piston, their areas from heights and widths; a crankcase volume pumped by the piston's
underside, fed through a reed (a pressure-opened valve) or a rotary disc; scavenging as a blend of displacement
and mixing with a short-circuited share; the charge's purity from it. **The expansion chamber** from card G's
ducts: header, diffuser, belly, baffle cone, stinger, the cones as short stepped ducts (state how many steps a
cone needs before the sound and the tuned speed stop moving). A power valve (the exhaust port's height against
speed). Singles, twins, triples. *Tests*: the trapped mass peaks where the pipe's returning wave says (tuned
length against speed, within 15 %); "on the pipe" shows as a torque band; four-stroking at light load (irregular
firing from poor scavenging) appears without being scripted; the null test untouched for four-strokes.

## D2. Heavy diesels

*Owns the diesel parts of `sim/combustion/`, truck parts of `sim/vehicle/`, by agreement with H and I.* A
compression-release brake through card E's auxiliary exhaust event with the fuel off; an exhaust brake (a flap
slot in card G's chain; or card F's vanes); compound and variable-geometry turbos as card F built them, in truck
sizes; injection timing, pressure and pilot from card H's fields, tuned to a mechanical-pump clatter and a
common-rail's quiet; over-fuelling and a smoke number; the eighteen-speed's splitter and range as real controls;
trailer mass and grades; the air system's sounds (the dryer's purge, the brakes' release) and the fan. *Tests*:
the brake's retarding power against speed in a plausible range, its bark's lines at the firing frequency; a
loaded rig's time up a grade against its power and weight.

## B2. Motorcycles

*Owns bike parts of `sim/vehicle/` and bike builds, by agreement with D and I.* Layout presets (V4s with their
firing orders, a boxer twin, triples); the vehicle as a bike (one driven tyre, a high centre of mass on a short
wheelbase: card I's wheelie limit is the launch; chain drive with sprockets and lash; a slipper clutch; a
quickshifter and auto-blipper through card H's cut and blip); the intake's dominance in the mix (card L). *Tests*:
launch limited by wheelie on a superbike, by grip on a cruiser; top speeds and quarter miles for three bikes
within the usual 8 %.

## V1 and V2. The views

*Own `ui/views/**` and `shaders/**` (new files), split as below.* On card J's kit.
- **V1**: *Build*: the cutaway that runs (a shader fed crank angle, the layout, valve lifts, pressures and
  temperatures as a small texture; the reference's `layout.sc` is its seed), glowing headers from wall
  temperatures, flame at a tail on a bang. *Exhaust*: the system as built, with the Scope's pressures along it
  moving. *Drive*: the dash (the reference's `cluster.sc`, grown: boost, lambda, temperatures, shift lights from
  the ECU), a side view of the car that squats and spins its tyres, the strip's tree and time slip.
- **V2**: *Valvetrain*: the cam editor (both lifts by angle from the Scope, overlap shaded, handles for
  centreline, duration and lift, the phasers' ranges), the flow bench. *Boost*: the compressor map from card F's
  Scope arrays with the operating point's trail. *Tune*: the maps with the live cell, baking, limiter and launch
  settings. *Dyno*: runs, overlays, the predicted curve from card C's runner. *Scope*: pressure against angle and
  against volume, port pressures, the output's wave, a spectrum against speed. *Garage*: builds, presets, compare.
- *Tests*: each view headless with scripted input (fingerprints, PNGs), each shader on the GPU path under Xvfb,
  a contact sheet for your eyes.

## P. Content, validation and the manual

*Owns `content/**` (builds and validation targets), `docs/manual/**`.* Archetype builds, generic names, each a
whole (engine, car, tune) that starts, idles, pulls and stops: about thirty across the families. For each, the
validation targets of card C's framework with sources. The manual: a page a screen, a page a subsystem's model
(from the `docs/models/` each sim card wrote), the parameter tables generated from the schema.

## W. The wheel and force feedback

*Owns `shell/input/wheel/**`.* Card B's seam filled: devices through SDL's joystick and haptic interfaces, a
mapping with a learn mode, range and centring; the G29's lights and whatever SDL lacks from TestingPlayground's
JUCE-free driver logic. Feedback from the model: the engine's shake by order (card D) as a vibration, the
limiter's buzz, wheelspin and lock-up from card I, a shift's kick. None of it testable here on hardware: a
made-up device for the logic, your wheel for the truth.

## R. Release

*Owns `packaging/**`, CI's release jobs.* Bundles for the three systems (macOS: the entitlement for the JIT,
signing and notarisation with your identities, by you or with secrets you provide; Windows: a folder or an
installer; Linux: an AppImage), licences collected (GPL-3.0 for the app; each dependency's notice), a first-run
experience (the compile's splash, a default build), settings polish, crash-safe files.
