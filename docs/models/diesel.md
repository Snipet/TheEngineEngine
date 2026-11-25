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
  compensator (`ecu.afc`, an equivalence ratio; 0 removes it) caps it at what the air there allows: 0.77 by default,
  lambda 1.30 (cited: a patent's "typically about 1.3", US 7,270,089; a second's axis lambda 1.27-1.37, US 6,508,241;
  not a maker's calibration; 0.8 until 25 Nov). It binds where the air is short (spooling, a lug), and there it moves settled
  points too. Moving it from 0.8 to 0.77 (25 Nov): diesel-v8-truck's most power 3120 N m / 535 kW -> 3016 / 517 kW at
  1637 rpm (-3.4 %): its `fuelBoost` asks for more than the reference turbo gives there, so the cap binds at its own
  rated point (open: the fuel curve, or its turbo, to put right); the tractor archetype at 1100 rpm 2117 -> 1929 N m
  (-9 %, boost 90 -> 79 kPa: the air short at the edge near 1060 rpm, less fuel and so less boost; a real engine holds
  its peak there on some twice that boost: the low-speed vane loop's, card N2's); the 2.0 at 1800 166 -> 159 N m (-4 %,
  18 kPa of boost), the same kind; every other take-up and rated point within half a per cent. Past 0.85 the
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

### With card H's fuel model: the four combinations

| `ecu.governor` | `fuel.model` | Fuelling |
|---|---|---|
| 0 | 0 | the reference's: 0.75 x lever of the air, its timing and burn |
| 0 | 1 | H's: the reference's law with H's `phiAt` (its `overFuel` raising the smoke limit), `soiAdd`, `pilot`, `rail` shaping when and how it burns; the smoke number from `phiAt` |
| 1 or 2 | 0 | D2's: the governor's rack, the fuel metered by it (`phiMetered`), the reference's timing and burn |
| 1 or 2 | 1 | one law: D2's `phiMetered` decides how much fuel (H's `overFuel` scales the rack's full fuel, past the compensator only when `afc` is 0 or above it); H's `soiAdd`, `pilot`, `rail` decide when and how it burns |

`tel.ecu.smoke` is D2's (card H stopped writing it): written on a diesel under either D2's governor or H's fuel model.

## The compression-release brake (`ecu.engineBrake`, `ecu.brakeSwitch`)

Fuel off; a slave piston opens the exhaust valve near the top of compression: the work of compression is thrown
away into the manifold (Jacobs, 1961). The lobe is card E's second exhaust event's shape (`valve.auxOpen`, `auxDur`,
`auxLift`), opened per cylinder by `HeavyDiesel::brakeArea` whatever `valve.model` is (`valve.auxOn` stays off: that one
is a cam always on). Area: two exhaust valves of 0.3 bore (or `ecu.brakeValve`), curtain pi d L, discharge 0.65
(judgement: a low lift's), sin^2 over the event with its flanks steepened (a hydraulic slave). Stages: a third, two
thirds, all of the cylinders (2, 4, 6 of a six), each joining or leaving only while its lobe is shut. It acts with the
pedal up, the clutch in, the engine running above `ecu.brakeMin` (900 rpm by default).

**Where the published half is.** At 2100 rpm, High, a lobe opening 30 degrees before the top, 3.5 mm:
268 kW with the manifold at 2.2 to 2.4 bar absolute (the reference turbo's gate holds 140 kPa at most, and card F's
machine without its vanes commanded shut held the same); 56 bar at the release. The ideal of throwing away the
compression's work, p1 V1 / (n - 1) (CR^(n-1) - 1) a cylinder a cycle at 2.4 bar and 2.48 L, 1.35, 17:1, is some 300 kW
for the six at 2100 rpm, and the motoring losses add 60: the model is within 25 % of its own ideal (the walls take
some of the squeeze's heat; the release is not instant, the first degrees after the top give a little back). The rest
is boost: a real brake shuts its turbine's vanes and runs 3 to 3.5 bar absolute in the manifold; scaled by that the
same engine gives some 400 kW, the published figure's range. Sweeping the opening: 660, 675, 690, 700 (3.5 mm) gave
234, 257, 267-272, 252-276 kW. The builds keep 25 degrees and 2.5 mm (some 10 % less): with 30 degrees and 3.5 mm
the bark loses its lines (at 2000 rpm the bins between the firing harmonics come up to them: the releases scatter
from cycle to cycle; not yet understood). **The vanes shut by the brake** (boost.model 1, variable vanes; `HeavyDiesel::vaneCommand` into
`Boost::vanesAndPurge`, card F's brake position: vanes to their stop, the gate held shut, the shaft's over-speed relief
still acting). At 2000 rpm, High: the matched turbo (size 1) over-speeds (12,417 rad/s against its 11,932 limit), its
relief opens the vanes to 0.78 and the manifold stops at 2.48 bar, 257 kW; size 1.3: 2.84 bar, 282 kW; size 1.6: 3.04
bar with the vanes shut, 294 kW (motoring 61). So the boost comes, but the power follows it less than the ideal says
(+25 % manifold, +5 % power) with the reference's exhaust, whose turbine's back pressure does not reach the cylinders.
With card G's network (exhaust.model 1) it does: size 1.6, High, 2000 rpm: manifold 3.41 bar, before the turbine 5.86
bar, **406 kW** (motoring 98 kW: the vanes at their stop): the published ~450 at 2100 rpm. The missing half was the
vanes and the turbine's back pressure. **But** (24 Nov, card G's aaedbf4): 45 % of that turbine's gas by mass was
phantom, a choked release's very large simple-wave gain in the pipes; with the valves' mean flow closed the pipe gets
1.03 times what the cylinders let out. On real mass size 1.6 makes 240 kW at 1.8 bar; the matched size 1 is the turbo
again (below, "On D's gas and G's closed exhaust"). Not yet done: the pressure-volume loop of one
braking cylinder against one motoring from the Scope (the picture asked for).

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

## A truck build's gearing

With card I's vehicle (`car.model` Vehicle) and the eighteen-speed (`car.gearbox` "Eighteen-speed"), the final drive is
the reference truck's rule (chosen from the engine's redline and the tyre) unless `car.autoGearing` is false: then
`car.finalDrive` is used (3.55 in mine). The eighteen ratios are Roadranger's (Eaton RTLO-18918B), whatever `car.ratio`
holds. The paddles do nothing with it: the lever, `BTN_SPLITTER` and `BTN_RANGE` drive it.

## The truck's parts (standalone, not yet in the engine)

`sim/vehicle/roadranger.fm`: the eighteen-speed (Eaton RTLO-18918B's ratios): lever positions L (5) and 1 to 4, the range
(`ctl.buttons` bit 32, held: high) and the splitter (bit 16): low L -> 1 + split, low k -> 1 + 2k + split, high k -> 9 + 2k +
split. The main box has no synchromesh: its dogs go in within 8 rad/s (some 80 rpm: judgement, drivers' 50 to 100);
the input shaft is the crank's with the clutch in, coasts down at 30 rad/s^2 with it down, the clutch brake stops it at
300 rad/s^2 at rest. Out of the window it grinds: clashes at 12 dogs x the slip (judgement), as long as the lever is
pushed there. The range shifts as the lever passes neutral (synchronised); the splitter, preselected, when the torque
breaks. `sim/vehicle/truckair.fm`: 120 L of tanks charged by 0.5 L of free air a crank turn; the governor cuts out at
8.3 bar (the dryer's purge, a 0.6 s hiss) and in at 6.9; a brake's release hisses at the relay valve; the spring
brakes' release blasts, their setting exhausts. Levels are judgement. In the engine with `spec.car.truckAir` and card
L's listener (sound.model 1): the purge at `LS_PLACE_RIGHT` x 10, the brakes at `LS_PLACE_REAR_AXLE` x 6 (my gains:
at L's valvetrain scale alone a purge was lost under an idling 15 L six). Measured: a spring-brake release lifts the
mix ~1 dB over 0.5 s (exhaust camera, idling); a purge (36.7 s after a start motoring at 2000 rpm) leaves L's
limiter at 0 dB (it has 18.7 dB of headroom; lower the purge, not the brakes, if it ever drives it below -3 dB).
The grind waits on card I taking the eighteen-speed's gear into its gearbox (the hook is asked through the lead).

## After card D's walls counted once (9e3a44f), the trucks on card H's fuel model

With `fuel.model` Fuels a diesel keeps 0.98 of its fuel's heat (the walls taken once, by Woschni's), not the
reference's 0.62. My truck builds (truck-15l-six, bobtail, the V8) moved to it. Measured, truck-15l-six: most torque
**2760 N m** (Cummins ISX15's published 2780, 2050 lb ft) and most power 548 kW at 2121 rpm — above the published top
rating (~450 kW, 605 hp): the governor's full rack is flat to the rated speed, where a real controller shapes the torque
curve (not modelled). The 36 t rig: 0-50 km/h 17.6 s; 6 % at 56.8 km/h on 400 kW (power and weight: 54.9); down 6 %
braked to 32 km/h, free to 53.5 (the redline). The compression brake, High, 2000 rpm: 249 kW with the reference turbo
and exhaust; 404 kW with the vanes shut and card G's network. Cold starts unchanged but one: governor Reference with
H's models 1 at 233 K now takes 12.7 s of cranking (1.1 s at 253 K). The table below is current (24 Nov evening).

## On D's gas and G's closed exhaust (24 Nov evening)

Three changes under the engines: D's single-zone gamma with a lean term (8b20b176), then a diesel's charge kept as air
until it burns (b551cd0: the first alone made a diesel compress with products' gamma, some 50 K short at the top, and
the slowest cold start went from 12.7 to 18.3 s); G's exhaust valves' mean flow closed (aaedbf4: the phantom gas
above); N2's surge guard re-derived on real mass. The brake's VGT position now also governs the shaft
(`vaneCommand(d16, shaft / omegaMax, vaneMin)`), as a production brake mode does it in the vane command rather than
leaving it to the machine's 105 % relief. Its plant, measured with the vanes held at 2000 rpm on High: the brake
bootstraps itself (more boost, more gas, more turbine), so at vanes 0.3 to 0.5 the shaft runs away to the relief and
back every 3 s; at 0.6 it creeps to some 11,300 rad/s; at 0.8 it settles at 6000 (omegaMax 11,932). A PID past 95 %
hunted into the relief (a lead term on the noisy rate and the integral both). A proportional band, opening from the
stop at 78 % of the limit to wide at 109 %, slewed at 2 a second, settles at 0.93 of the limit with the vanes at 0.63:
282 kPa in the manifold, 346 kW, the shaft at most 11,714 rad/s on the way up. Why the gain flips (N2): at a fixed inlet pressure the
turbine's power rises with the nozzle area, some 3.5 times its power a unit of vane; but on a brake the release into
the manifold is fixed, so the inlet pressure falls as the vanes open and past about 0.4 the power per step is flat to
falling. The vanes' actuator in card F's machine is a 50 ms lag; real electric ones take 100-300 ms a stroke.

## The full-load fuel curve (`ecu.fuelCurveRpm`, `ecu.fuelCurveMg`; 25 Nov)

A governed diesel's full-load torque curve is its fuel curve: the maker shapes torque speed by speed with the fuel at
full rack. Eight pairs, rpm and mg a stroke a cylinder at full rack, in the fuel's own units (a maker's
injected-quantity table fills it directly); used where mg > 0, linear between, held outside; none set: `fuelBoost`'s
flat line, to the bit. The over-fuel allowance and the compensator act on it as on the flat line, so it cannot give
torque the air does not support. Where a build derives it from torque, the conversion is the build's own model torque a
milligram (truck-15l-six: 8.94 N m/mg at 1600 rpm, fuel-limited), so the curve absorbs the model's efficiency; its
torque and power against speed are then set by the curve, not evidence, and what still tests the model is what the
curve does not set: BSFC (the truck 179-189 g/kWh; P's tractor archetype 202-211, against a 15 L's published minimum
near 185-195, P to source), the boost and lambda at peak torque and rated, the take-up.

truck-15l-six (and its bobtail): the class's cited 2779 N m at 1000 rpm held to 1400 and 451 kW at 1800 and 2100
(cited), 75 and 88 % of the peak at 700 and 800 (judgement). Held, heat settled, before -> after: 1100 rpm 2671 ->
2660 N m (lambda 1.30: air-limited on the reference turbo's 140 kPa, the curve's low points not showing), 1300 2718 ->
2715, 1600 2759 -> 2703, 2100 2729 N m / 600 kW -> 2135 / 470 kW. The tractor archetype's curve is proposed to card P,
who commits it with its validation rows moved to "set by the fuel curve".

## The idle's clatter: a hump, not a comb (24 Nov, late)

Card P's idle row found the three diesel archetypes with a line 1-10 dB under their exhaust's note at a warm idle:
the reference's clatter (sim/sound/extras.fm: six panel modes, 900 to 4800 Hz, lower by the cylinder's size to the
-1/6), struck at the start of combustion at the same moment every cycle, ringing as sharp firing harmonics (the 2.0 on
its 1900 Hz mode, the pickup's on 1661, the tractor's on 1454 and 1989). The tractor's and the pickup's exhaust note
is also some 20 dB quiet at idle: the VGT's vanes sit near shut there and reflect the pulses (G's measure; N2's). Under
the physical diesel (fuel.model 1) the start of combustion now takes the ignition delay's cycle-to-cycle spread, a
standard deviation of `socSpread` (0.150) times the model's own delay (`DieselFields::socSpread`, chargeDiesel): at the
tractor's warm idle (3.60 to 3.73 degrees of delay) some 0.55 degrees. The spread's first form took the noise's
gauss() for a unit sigma; it is 0.577 (three uniforms), so it gave 0.58 of the cited spread until 25 Nov (now x sqrt 3).
Measured (ClatterTest, the mechanism: the start of combustion's sigma over 200 charges at the tractor's warm idle):
0.498 degrees against 0.529 expected, 0.119 without it. Card P's diesel row (the comb of firing harmonics 1-3 kHz over
the spectrum between them, in the mix, at the archetypes' own settings, at most 8 dB): tractor 1.9 dB, pickup 3.2, the
2.0 3.5 (6.4 to 7.0 before the correction). The comb in the mix moves with every valve and turbine change, by much:
without the spread 13.2 dB before 68e683f1, 13.1 after it, 5.5 after 6771c402 (four-valve curtain), 11.0 after
7e9bee72 (N2's map), 7.0 after 29684891; so the test is of the mechanism, and the sound is P's row. A cold start's long delay varies more: the cold starts on H's models take longer (233 K, the
mechanical governor, 5.2 to 9.4 s; 253 K up to 2.5 s), within their bounds.

**Open**: the spread's size is read but not of the right quantity. Szpica and Kluczynski, Energies 2026, 19, 1343 (read
by card P, docs/measurements/2026-11-25-0032-judgement-sources.md, 2), Table A3: a Perkins 1104D-E44TA at idle (800
rpm, no load, 500 cycles), the crank angle of peak pressure, sigma 0.559 degrees (range 1 to 4, about 1 degree's
resolution); k = 0.559 / 3.73, the tractor's idle delay when it was set. The peak's angle holds the burn's own
variation too, so it likely overstates the start of combustion's; no measured spread of the start of combustion or the
ignition delay at idle has been found. Its growth with the delay: the trend of a Fischer-Tropsch-fuelled diesel's
study (Appl. Sci. 2019, doi 10.3390/app9102032), unread.

## Measured (tee-tests-diesel; the 15 L six, 300 K, 101325 Pa; 24 Nov evening)

| What | Measured | Compared with |
|---|---|---|
| Rated, truck-15l-six (H's fuel model, electronic governor) | 2783 N m, 553 kW at 2121 rpm | Cummins ISX15 2780 N m (2050 lb ft); top rating ~450 kW (no torque shaping modelled) |
| Other builds | V8 truck 3125 N m / 536 kW; mechanical classic 2170 / 436; tuned pickup 1214 / 228; marine V12 3423 / 593 | |
| Compression brake, 2000 rpm, stages 0/1/2/3 (reference turbo) | 63 / 115 / 180 / 254 kW; with the flap 311 | 15 L Jakes rated ~450 kW at 2100 (Jacobs, makers' brochures) |
| Same, High, 1200 / 1600 / 2000 rpm | 118 / 182 / 254 kW | rising with speed, as published curves |
| VGT brake, N2's matched turbo, G's network, High, 2000 rpm, after a climb, warm | 365 kW; manifold 319 kPa over a second; shaft at most 9,673 rad/s (omegaMax 10,149), vanes 0.68 | matched VGT brakes 2.5-4 bar (recalled, not cited) |
| Bark against motoring | harmonics 2-10 of 100 Hz ~25 dB over the bins between | |
| Exhaust flap, 2000 rpm | stand-in 3.0 bar, 131 kW; G's butterfly 358 kPa, 146 kW (open 63) | makers' 2.5-4 bar; 110-190 kW |
| Droop line (both governors) | rack 1.00 / 0.50 / 0.00 at rated, half way, high idle | |
| Idle | mechanical 640-809 rpm (169 swing); electronic 681-685 | |
| Pilot 4 %, 1200 rpm full load | rise 15.6 -> 8.0 bar/deg; noise 244 -> 179 Pa | a pilot ~halves it (Heywood ch. 10) |
| Injection 6 degrees later | rise 8.1 bar/deg | |
| Over-fuel 30 %, no limiter, 1400 rpm | 2780 -> 3326 N m; 1.3 -> 7.2 FSN | |
| Tuned pickup floored at 1600 rpm | 10 FSN while spooling (25 kPa), 1.4 spooled; stock 1.8 / 0.1 | |
| Cold start (all governors, H's models 0 and 1; asserted per case) | 253 K: 0.5-1.9 s cranking; 233 K: 1.1-5.2 s | |
| 36 t rig | 0-50 km/h 17.6 s; 6 %: 57.8 km/h at 407 kW (power and weight: 55.8); down 6 % from 45 km/h: free to 53.4, braked to 31.6 | 36-40 t, 400-450 kW: 45-60 km/h on 6 % (judgement) |
| Card F's turbos at 1300 rpm | big single 208 kPa, 90 % in 2.80 s; VGT 209 kPa in 2.60 s; compound 213 kPa in 1.90 s | |

**Limits**: the reference turbo limits low-speed torque (peak 2000 N m at 1400-1600 rpm, not 1000-1200); a 15 L's
brake sits at the low edge of published figures. The model takes at most 30 L and 16 cylinders, a diesel's redline
at about 13.5 m/s of piston speed, an idle of at least 350 rpm: a 27 L marine V12 at 1800 rpm is the largest
(`diesel-marine-v12`); a medium-speed locomotive or ship's engine (100-200 L, 1000 rpm) is past it.
