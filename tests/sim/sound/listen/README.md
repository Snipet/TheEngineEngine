# Card M's listening scenarios

Engine scenarios (`tee-engine-scenario/1`) for the owner's listening pack. Each sets `sound.view` 3 (Engine) and
`spec.voices.model` 1, and turns one voice from off to on part way, so the same take has the before and the after.
Build `v8` unless said; the two gate files need `content/builds/boost/turbo-four.json`. Render:
`tee-render --engine --build v8 --scenario tests/sim/sound/listen/m-whine.json`.

Loudness is as the model has it, not raised for the pack (but `m-belt`, +6 dB): measured in this view the voices sit
6 to 18 dB under the reference's own engine bay, itself some 35 dB under the mix. Meant to be only just heard: the
belt and alternator, the toothed cam belt, the hydraulic engine (nothing added), the fuel pump while running, the
helical whine at light load. Meant to be plainly heard: straight-cut whine, gear-driven cams, a dog box's rattle,
solid lifters with wide lash, direct injection at idle, a locked viscous fan at high revs, the starters, the
external gate to the air.

| File | What to listen for |
|---|---|
| m-whine | Third gear cruising (~2200 rpm): from 10 s a thin whine near 700 Hz (19 teeth x crank) and the axle's lower one; from 15 s straight-cut: louder, buzzier |
| m-rattle | Sequential box in neutral, clutch up: from 6 s a dry metallic rattle at idle; gone while the clutch is down (11-14 s) |
| m-cam-drive | 3000 rpm: the chain's faint whine; from 7 s gear-driven cams' whine at 30 x crank (~1.5 kHz); from 12 s the quieter toothed belt |
| m-lifters | Idle: from 6 s a crisp tick at every valve; louder from 11 s (0.5 mm lash); at 4000 rpm from 16 s |
| m-direct | Idle: from 6 s the sharp double tick of direct injectors and the cam pump |
| m-fan | Electric fan on 5-10 s (blade tone ~280 Hz and rush), coasts down; viscous fan locked from 14 s, roaring with the revs from 17 s |
| m-starter | Key on: the pump's prime (~2 s whine ~3.9 kHz) before anything; direct-drive starter (growl); then key on, prime, a gear-reduction starter (higher whine); the solenoid's clunk and the run-down after each |
| m-belt | 2400 rpm: alternator whine (100.8 x crank, ~4 kHz) and the pumps, off 5-9 s, on after |
| m-gate | Full load at 5500 rpm on the dyno: from 9 s the internal gate's metallic chatter |
| m-gate-dump | External gate: into the exhaust, then from 9 s vented to the air: a raw rasp pulsing with the firings |

Nobody here has heard any of it.
