# Valvetrain

The cams and valves: how far each valve opens at each crank angle, how much air that lets through, and the things
that move the cams (phasers, two-stage cams, continuous lift, cylinder deactivation) and limit them (valve float).

Valvetrain model (`valve.model`): **Lab** is the model below, used by the shipped archetypes. **Reference** is the
original simulator's single cam dial (one number for both cams' duration and separation), kept so the two can be
compared.

## What is simulated

- **Cams as a cam card gives them**: duration at the seat, duration at 1 mm of lift, lift, centreline and lobe
  "fatness". The generator makes a smooth lift curve that meets the asked durations within a degree and the lift
  within 1 %. "At the seat" is the standard 0.15 mm (0.006 in) checking height.
- **From lift to flow area**, with the standard shape: the valve's curtain area at low lift, flattening once the lift
  passes about a quarter of the valve's diameter. Port quality scales it.
- **A flow bench** (CFM at 28 in of water against lift) worked out from the same areas, as a head porter would measure.
- **Phasers**: the intake advances, the exhaust retards, at a rate that depends on oil pressure and temperature; parked
  when the oil is cold or without pressure.
- **Two stages**: a second profile above a set rpm, with hysteresis. Each cylinder changes only while its valve is
  shut, so the change goes round the engine within a cycle without a step.
- **Continuous lift** (intake only, as on throttle-less systems): lift and duration shrink together; it can be the
  load control in place of the throttle.
- **Cylinder deactivation**: chosen cylinders rest at light load inside an rpm window, valves shut and fuel off.
- **Free valves**: opening and closing given outright (late or early intake closing), and a second exhaust event.
- **Valve float**: the rpm where the spring can no longer keep the follower on the cam, from spring rate, preload,
  the valve's and train's mass and the cam's sharpness. Past it the valve bounces and leaks, and power falls steeply.

## What is left out

- Asymmetric lobes, valve lash, valve-to-piston contact (VALVES shows the clearance as an estimate only).
- A rotary engine keeps the original simulator's ports.
- No damage: floating valves lose power and clatter, nothing breaks.

## What the numbers mean

| Setting or reading | Meaning | Typical |
|---|---|---|
| Duration at the seat / at 1 mm | Crank degrees the valve is open at all / open by 1 mm | 1 mm: 200 to 230 street, 260+ race; seat figures some 35 to 50 more |
| Lift | Peak valve lift | 9 to 13 mm |
| Centreline | Crank degrees from the overlap top centre to the lobe's peak | Intake 100 to 115 |
| Lobe separation | Half the angle between intake and exhaust centrelines | 106 to 116 |
| Overlap | Degrees both valves are open round the overlap top | Small for smooth idle, large for top-end power |
| Port quality | Flow of the port against the original simulator's | Defaults 1.35 intake, 1.5 exhaust |
| Spring rate, preload, valve mass, valvetrain kind | Set the float speed | |
| Float speed | rpm where float begins; shown on VALVES | Above the redline in a healthy build |
| Intake/exhaust phase | Degrees moved from the parked position, asked and actual | |

## Known gaps today

- **The compression brake has no switch yet.** The second exhaust event can be set, but it then acts on every cycle,
  and an engine with it on does not start. A brake switched by the driver is being built for the heavy diesels.
- The float's bounce and seat leak are sized by judgement; the cliff (21 % less torque at 1.1 times the float speed,
  64 % at 1.25 times) has the right shape but has not been checked against a real engine.
- The default valve diameters, phaser rates and stage conditions are judgement from typical engines.
- The flow bench figures for a small-block head (172 CFM at 0.500 in at the original simulator's port quality, 232
  at the default) bracket real stock heads (190 to 230 CFM) from memory, not from a checked source.
