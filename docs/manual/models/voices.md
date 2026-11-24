# Mechanical voices

The sounds of the machinery around the combustion: gears, belts, fans, pumps, injectors, lifters, the wastegate and
the starter. Each is driven by what the model is doing (a shaft's speed and tooth count, a load, a temperature, a
valve event), not played from a recording or a loop.

Mechanical voices (`voices.model`): **Voices** is the model below. **Reference** is the original simulator's engine
bay sounds, kept so the two can be compared.

## What is simulated

| Voice | Driven by | What it is |
|---|---|---|
| Gear whine | Gear, input speed, torque | Each gear pair's tooth-meshing frequency and two harmonics, louder with load and speed. Straight-cut gears 12 dB louder than helical. A direct gear (1:1) carries no mesh. Reverse's idler. |
| Final drive whine | Propshaft speed | The pinion and ring gear's mesh. |
| Neutral rattle | The crank's own firing ripple | Loose gears knocking within their backlash in neutral with the clutch up; strongest on a dog box, absent on an automatic. |
| Accessory belt | Crank speed | Alternator, water pump and power-steering pump at their own ratios and vane counts; belt hiss. |
| Cam drive | Crank speed | Chain, toothed belt, or straight-cut gears. |
| Supercharger drive | Crank speed, boost | A blower's belt or gears. |
| Cooling fan | Coolant temperature, crank, key | Viscous fan (slips cold, locks hot) or an electric fan switching on and off. |
| Fuel pump | Ignition, crank | Primes for two seconds at key-on, then runs while the engine turns. |
| Direct injectors | Each injection | The tick of a petrol direct injector opening and closing, and the high-pressure pump. |
| Lifters | Valve events | Solid lifters' lash tap, stronger with more lash; hydraulic lifters add nothing. |
| Valve float | The valvetrain's float | Extra bounces after each landing. |
| Wastegate | Gate opening, exhaust flow | An internal flap's chatter when part open; an external gate venting to the air. |
| Starter | The key, crank | Pinion and ring gear, motor commutator, solenoid knock and pinion strike, run-down. Direct drive or gear reduction. |

The intake's own sound (the air box snorkel or the throttle trumpets) comes from the [intake model](intake.md) when
the Induction model is Lab.

## What is left out

- Spider gears in the differential (they need cornering).
- Each voice has its own output for the listener to place in the car (gearbox, differential, front of engine, head,
  tank, turbo, starter); until the listener places them, they are mixed into the engine bay.

## What the numbers mean

| Setting | Meaning |
|---|---|
| All voices | Overall level of the voices |
| Gear teeth | By gearbox, Helical, or Straight cut |
| Cam drive | Chain, Toothed belt, Gears |
| Cooling fan, Fan comes in at | None, Viscous clutch or Electric; the coolant temperature it comes in at |
| Injection | Port or direct |
| Lifters, Lash | Hydraulic or solid; the gap for solid lifters (typically 0.2 to 0.5 mm) |
| Wastegate, External gate vents | The gate's voice; an external gate venting Into the exhaust or To the air |
| Starter, Starter power | Direct drive or gear reduction; its power (about 0.8 to 3 kW) |

Every voice line was measured within 0.5 % of its computed frequency (for example the alternator at 100.8 times crank
speed, the gearbox's constant mesh at 19 times crank speed in third). In a V8's mix the voices sit some 6 to 18 dB
under the original engine bay sounds.

## Known gaps today

- Every level is judgement, set beside the original simulator's; nobody has listened to them yet. Listening is the
  check that matters.
- Where each voice is heard from waits for the listener's placement.
