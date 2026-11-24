# Heavy diesels (card D2)

The heavy diesel's controls and brakes, what they model, the numbers and where each comes from, and what is
judgement. Code: `sim/combustion/heavy.fm` (`HeavyDiesel`), two gated lines in card H's `chargeDiesel` and fields in
H's `DieselFields` (`sim/combustion/diesel.fm`); Spec: the D2 block at the end of `spec.ecu`; telemetry: the D2 block at
the end of `tel.ecu`. Tests: `tee-tests-diesel` (`tests/sim/diesel/`, ctest `diesel.*`). Builds: `content/builds/truck-*`,
`diesel-*`. Every field of D2's at its default is the reference to the bit (the null test).

## The governor (`ecu.governor`)

The reference fuels a diesel by its air: an equivalence ratio of 0.75 times the lever whatever the boost, so it can
never smoke, and its lever has an idle floor of 0.12. A real pump meters fuel by the rack. With a governor:

- **Fuel by the rack.** `fuelFull`, the fuel a stroke at full rack, is an equivalence ratio of 0.75 of the air a
  cylinder holds at the boost the pump was set up for (`ecu.fuelBoost`, at 320 K, 0.92 filling). The boost
  compensator (`ecu.afc`, an equivalence ratio; 0 removes it) caps it at what the air there allows. Past 0.85 the
  fuel finds less oxygen: the heat levels off toward an equivalence ratio of 1 (0.85 + 0.15 (1 - e^-(phi-0.85)/0.15)),
  the rest is soot. The smoke number from what was injected: 0 at 0.6, 4 FSN at 0.9 (4 x^2, x = (phi - 0.6) / 0.3),
  at most 10 (judgement: the shape of published FSN against air-fuel ratio, as card H's `phiAt`).
  So "rolling coal" is a consequence: take the compensator off or over-fuel, and the fuel outruns the air while the
  turbo spools.
- **Mechanical** (1): an all-speed governor: the pedal sets a speed from idle to high idle; flyweights feel the speed
  with a 40 ms lag; the rack aims at (set - felt) / (droop x rated); it is a mass on the flyweights' spring (4 Hz,
  damping 0.25) with friction that holds it within 1.5 % of its travel. At idle it hunts.
- **Electronic** (2): the pedal asks for fuel (min-max); a PI idle governor (gains 1.5 and 0.8 a second on the speed's
  share of idle); the droop line caps the fuel toward high idle.
- High idle: rated (the layout's redline as the reference sets it) x (1 + droop), or `ecu.highIdle`. Bosch's governors
  are specified with 5 to 10 % droop; 6 % is the default (judgement).
- Starting: the reference's start fuel (0.45 of the rack while cranking, tapering over a second).

## The compression-release brake (`ecu.engineBrake`, `ecu.brakeSwitch`)

Fuel off; a slave piston opens the exhaust valve near the top of compression: the work of compression is thrown
away into the manifold (Jacobs, 1961). The lobe is card E's second exhaust event's shape (`valve.auxOpen`, `auxDur`,
`auxLift`), opened per cylinder by `HeavyDiesel::brakeArea` whatever `valve.model` is (`valve.auxOn` stays off: that one
is a cam always on). Area: two exhaust valves of 0.3 bore (or `ecu.brakeValve`), curtain pi d L, discharge 0.65
(judgement: a low lift's), sin^2 over the event with its flanks steepened (a hydraulic slave). Stages: a third, two
thirds, all of the cylinders (2, 4, 6 of a six), each joining or leaving only while its lobe is shut. It acts with the
pedal up, the clutch in, the engine running above `ecu.brakeMin` (900 rpm by default).

## The exhaust brake (`ecu.exhaustBrake`, `ecu.exBrakeSwitch`)

A flap after the turbine (1) or the turbine's vanes shut (2: 1.2 x the limit). Its back pressure from the flow through
the closed flap's leak (`exBrakeLeak` of a pipe of 0.85 m^2 a m^3 of displacement; discharge 0.8), dp = (m / 0.8 A)^2 /
(2 rho), at most `exBrakeLimit` (a regulated flap's spring: 3 bar by default; makers' 2.5 to 4), filling over 0.12 s.
The cylinders' exhaust ports see it (half of it added to the header's arriving wave: a pipe's pressure is twice its
arriving wave). **Not modelled**: the flap as an element of the pipes (card G's flap slot is not in the engine yet), so
the waves past it, and its muffling of the tail, are as without it.

## Measures

`tel.ecu.riseRate`: cylinder 1's most rate of pressure rise in its last cycle (Pa a crank degree). `tel.ecu.pressureHf`:
the rms of its pressure's second difference a sample over that cycle (Pa: the high-frequency part that rings the
block). `tel.ecu.smoke`: the filter smoke number (with the governor, or with fuel.model 1). `tel.ecu.rack`,
`brakeCylinders`, `backPressure`, `governorSpeed`.

## Measured

(filled as the tests land: see the handoff, docs/handoffs/d2-diesels.md)
