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

## The truck's parts (standalone, not yet in the engine)

`sim/vehicle/roadranger.fm`: the eighteen-speed (Eaton RTLO-18918B's ratios): lever positions L (5) and 1 to 4, the range
(`ctl.buttons` bit 32, held: high) and the splitter (bit 16): low L -> 1 + split, low k -> 1 + 2k + split, high k -> 9 + 2k +
split. The main box has no synchromesh: its dogs go in within 8 rad/s (some 80 rpm: judgement, drivers' 50 to 100);
the input shaft is the crank's with the clutch in, coasts down at 30 rad/s^2 with it down, the clutch brake stops it at
300 rad/s^2 at rest. Out of the window it grinds: clashes at 12 dogs x the slip (judgement), as long as the lever is
pushed there. The range shifts as the lever passes neutral (synchronised); the splitter, preselected, when the torque
breaks. `sim/vehicle/truckair.fm`: 120 L of tanks charged by 0.5 L of free air a crank turn; the governor cuts out at
8.3 bar (the dryer's purge, a 0.6 s hiss) and in at 6.9; a brake's release hisses at the relay valve; the spring
brakes' release blasts, their setting exhausts. Levels are judgement. Neither is mixed into the engine's sound yet,
and the vehicle's gearbox (card I's) does not yet take the eighteen-speed's gear: both are asked (the handoff).

## Measured (tee-tests-diesel; the 15 L six, 300 K, 101325 Pa)

| What | Measured | Compared with |
|---|---|---|
| Compression brake, 2000 rpm, stages 0/1/2/3 | 56 / 104 / 164 / 239-247 kW | 15 L Jakes rated ~450 kW at 2100 (Jacobs, makers' brochures): at the low edge; the asserted 200-550 kW is judgement |
| Same, High, 1200 / 1600 / 2000 rpm | 114 / 173 / 239 kW | rising with speed, as published curves |
| Bark against motoring (exhaust only, below the compressor) | +8.4 dB rms; harmonics 2-10 of 100 Hz ~25 dB up, 20+ dB over the bins between | |
| Exhaust flap, 2000 rpm | 3.0 bar held, 117 kW (+61); with the Jake 318 kW | makers' 2.5-4 bar; 110-190 kW |
| Droop line (both governors) | rack 1.00 / 0.49 / 0.00 at rated, half way, high idle | |
| Idle | mechanical 637-781 rpm (145 swing); electronic 681-685 | |
| Pilot 4 %, 1200 rpm full load | rise 11.2 -> 5.4 bar/deg; noise 160 -> 61 Pa | DI diesels 3-10 bar/deg; a pilot ~halves it (Heywood ch. 10) |
| Injection 6 degrees later | rise 4.7 bar/deg | |
| Over-fuel 30 %, no limiter, 1400 rpm | 2019 -> 2446 N m; 1.3 -> 7.3 FSN | |
| Tuned pickup floored at 1600 rpm | 10 FSN while spooling (25 kPa), 4.4 spooled; stock 1.8 | |
| Cold start (all governors, H's models 0 and 1) | 253 K: runs after 1.0-1.9 s cranking; 233 K: 2.9-4.5 s | |
| 36 t rig | 0-50 km/h 19.3 s; 6 %: 42 km/h at 278 kW (power and weight: 39); down 6 % from 45 km/h: free to 63 (the redline), braked to 39 | 36-40 t, 400-450 kW: 45-60 km/h on 6 % (judgement) |
| Card F's turbos at 1300 rpm | big single 59 kPa, 90 % in 2.05 s; vanes 192 kPa in 0.65 s; compound 202 kPa in 1.9 s | |

**Limits**: the reference turbo limits low-speed torque (peak 2000 N m at 1400-1600 rpm, not 1000-1200); a 15 L's
brake sits at the low edge of published figures. The model takes at most 30 L and 16 cylinders, a diesel's redline
at about 13.5 m/s of piston speed, an idle of at least 350 rpm: a 27 L marine V12 at 1800 rpm is the largest
(`diesel-marine-v12`); a medium-speed locomotive or ship's engine (100-200 L, 1000 rpm) is past it.
