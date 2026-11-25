# The boost package, final table (25 Nov, for the diesel window)

The package: the ruled map (all-seven fit, one floor at tip Mach 0.67), (b) as built, the whole-flow VGT nozzle, the
vane guard removed, and the taper from the fuel curve where a build states one. Tree: n2-b-local at 668c642e; the
series is docs/handoffs/patches/n2-package.patch. Every number below was measured on that tree, as the mean of the last 30 of 60 one-second rows after the step
(steady). The first figures were the 6 rows ending 16 s after the step, partly the VGT loop's transient; only the two
cells marked moved by more than 1 % (rated-choke-q1.md).

## Wheels (the rule's)

| build | wheel |
|---|---|
| tractor | 72.4 mm |
| the truck's sets (D2's fixture) | 69.8 mm |
| 2.0 diesel | 38.4 mm |
| pickup | 64.7 mm |

## Cells

| build | rpm | torque N m | note |
|---|---|---|---|
| tractor | 900 / 1000 / 1100 | 2397 / 2580 / 2721 | |
| tractor | rated, on the taper (202 kPa) | 2425 (482 kW) | shaft 0.899, flags 0 |
| tractor | rated, flat 260 | | shaft 0.986-0.990 |
| truck's sets (target 260) | 1000 / 1300 / 2100 | 2507 / 2674 (2653 at 16 s) / 1867 | at 2100: shaft 0.962, choke flag 2.02 |
| 2.0 | 1750 | 266 (252 at 16 s) | VW's 320: -17 %, a miss; the back-pressure limit holds the vanes |
| 2.0 | 4000 | 299 (125 kW) | shaft 0.813, no flags |
| pickup | 1300 / 1700 / 2800 | 863 / 998 / 1019 (299 kW) | shaft 0.832 at 2800 |

## Validation rows (14 boosted archetypes): misses 45 -> 46

Against n2-b-local before the map:

| row | before | after |
|---|---|---|
| 2.0 peak power | pass | miss (130 kW, +0.9 % over the band) |
| flat-plane 3.9 peak torque | miss | pass |
| tractor header gas | pass | miss (694 K) |
| turbo four torque at 1500 | 196.8 (miss) | 208.5 (miss) |
| kei, rotary peak power | miss | miss, -3.4 % and -3.8 % (the fit's right end at the redline) |

Speed at peak torque moved on the 5-cylinder, the 3.0 twin and the hot vee, all inside their bands.

## Stated wheels against the rule's (more than 5 % apart)

Only the turbo four differs: stated 42.5 mm (the real IS20's, per P), against the rule's 46.5 mm (+9.5 %).

## Tests (on the package tree, the nightly tier)

| suite | result |
|---|---|
| boost | 34/34 (MapTest restated on the cited lines) |
| calib-boost | all pass |
| diesel | 25/28. TurboTest :88/:108/:109 (the old truck fixture: VGT 43.8 kPa against the single's 82.6) and BrakeTest :226/:234: D2's fixture and brake, which come in with the package |

## What a person should listen to

The map changes the whine's pitch at a given boost only through the shaft speed, and on the 2.0 and the truck that
moves by a few percent. Listen for:
- the truck at 2100 near choke: a steady whine that should not hunt;
- the tractor's rated pull on the taper: no surge chuff;
- the 2.0 at 1750: the spool through the climb should sound smooth and unchanged.

## Validation rows that read a transient (for P to re-time; nobody tunes to them)

The harness's default settle is 4 s. On these cells the engine is still spooling at 4 s: they moved by more than 0.2 %
or 0.5 kPa when only the spool's path changed (the shaft's ceiling, shaft-ceiling.md), and agree within 0.1 % once
settled for 60 s.

| build | rpm |
|---|---|
| boxer four turbo | 5000 |
| 2.0 common-rail diesel | 1500, 1900 |
| inline five rally | 3500 |
| kei 660 | 2000 |
| pickup 6.7 diesel | 1000, 1500, 1900 |
| straight six twin turbo 3.0 | 2000 (it also wanders 465-518 N m row to row), 3500 |
| tractor 15 L | 1000, 1500 |
| turbo four 2.0 | 2000, 3500, 6500 |
| twin-turbo hot vee 4.0 | 2000, 3500 |
| V6 3.5 twin turbo | 2000 |

The tractor at 1900 is not one of them: it is shaft-limited on a flat 260, and the ceiling moves it at steady (+2.5 %).
