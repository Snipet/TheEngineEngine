# (b) built into the matching, the setpoint cap, and TurboTest:118 (25 Nov, local; branch n2-b-local, fe4582fa)

## (b) as built (local commit fe4582fa, not pushed: sim/ frozen)

- **spec.boost.peakRpm ("Torque peak from", visible):** the makers' speed where a build states it. At 0:
  - the full-load fuel curve's lowest speed within 2 % of its most;
  - else 0.6 of rated.

  The engine passes it with the compression ratio (Boost::peakSpeed). The call sits in sim/engine.fm, D's file, two
  lines, so D's leave is needed for the push.
- **machine.fm:** for a governed diesel, the compressor's annulus is the smaller of two:
  - F's 1.15 at rated;
  - the largest wheel holding the climb at the torque peak 10 % right of the surge flag's line at every ratio from
    1.3 to the design ratio.

  The flow uses VE 0.9 x (1.4 rc - pe/pi) / (1.4 rc - 1) at the build's backPressure. That is Heywood's ideal-cycle
  form, ch. 6, as the lead recalled it and I rearranged it. **The page itself is not checked**: one web search found
  only the book's contents. Petrols, compounds and model 0 are untouched.
- **The wheels it builds (read from the model):**
  - tractor 83.9 -> 76.8 mm (peak 1000, from its curve);
  - the truck's sets 81.8 -> 74.7 mm (1000, from its curve);
  - the 2.0 39.4 -> 36.9 mm (1750 stated; 2400 derived would leave it unchanged);
  - the pickup unchanged at 65.4 (1700, or 1740 derived);
  - the combo large diesel unchanged at 56.4 (1520 derived, twin).
- **The other shipped diesels** (the V8 truck, the marine V12, the mechanical classic, the tuned pickup, the 15 L
  truck builds) run boost.model 0, the reference turbo. **(b) does not touch them.**
- The torque-peak speeds are set with `--set boost.peakRpm` in these runs. P states them in content.

## The table: today / (b) with the guard on / (b) with the cap, guard off

Holds are 30 s; cells give torque N m / boost kPa (the range of the 1 s means over the last 21 s). "From 2000" is
the truck approached from above. The cap column gives the cap's kPa.

| build, rpm | today | (b), guard on | (b) + cap, guard off |
|---|---|---|---|
| tractor 900 | 1324 / 24 | 1403 / 31 | 1402 / 31 (cap 23) |
| tractor 1000 | 1509 / 40 | **2579 / 225** | **2145 / 95 (66-101), cap 89** |
| tractor 1100 | 1911 / 76 | 2720 / 251 | 2720 / 251 |
| tractor 1300 | 2760 / 257 | 2761 / 270 | 2768 / 268 |
| tractor 1900 | 2207 / 260 os | 2256 / 222 os | 2255 / 222 os |
| take-up 1100 (t90) | 0.5 s (to 71 kPa) | 3.5 s (248) | 5.5 s (262) |
| truck 1000, from 800 / from 2000 | 730 / 717 | 845 / 881 | 669 / 671 (cap 1) |
| truck 1100 | 839 / 1150 | **1190 / 2112** (two states) | 878 / 955 (cap 8-12) |
| truck 1300 | 1522 / 1528 | **2160 / 2198** | **1083 / 1089 (cap 41)** |
| truck 1900 | 1997 / 1997 | 1705 / 1705 | 1709 / 1712 |
| pickup 1300 / 1700 / 2800 | 576 / 996 / 941 os | the same | 575 / 997 / 941; take-up 7.5 s against 3.5 |
| 2.0 1500 / 1750 / 2000 / 4000 | 144 / 155 / 176 / 284 | 151 / 158 / **329** / 282 os | 144 / 161 / 329 / 282 os |

"os" means the over-speed flag is set.

Full-load pulls at 25 rpm a 0.5 s (selected):

| pull | at | today | (b) |
|---|---|---|---|
| tractor | 1200 | 2020 | 2648 |
| truck | 1600 | 1450 | 1803 |
| truck | 1800 | 2253 | 1772 |
| 2.0 | 2200 | 192 | 318 |

## The cap latches at the tractor's torque peak and everywhere on the truck below 1900

- **What it does:** the cap holds the setpoint at the surge flag's ratio at the flow passing now (followed over
  50 ms), less (b)'s 10 % in flow.
- **Why it latches:** while the engine spools, the flow is low, so the cap is low, so the boost stays low and the
  flow stays low. The cap has no way to rise except through flow that its own limit withholds.
- **The tractor at 1000** (its torque peak, on (b)'s wheel) settles at 95 kPa under a 89 kPa cap, against 225 kPa
  without it. At 225 kPa the operating point is 1.03 of the knee (after-burn-cure.md), so the cap would not bind
  there. The steady state is reachable; the climb to it is not.
- **The truck** stays at a cap of 1 to 41 kPa from 1000 to 1300, from below and from above: 1083 N m at 1300
  against 2160 with the guard.
- **The pickup's take-up** doubles, 3.5 to 7.5 s.

By the lead's condition, the latch at the tractor's 1000 puts the opening test back on the table.

## The truck at 1900: what takes its torque on the smaller wheel

The ledger is from an instrumented copy, guard on, from 2000, today's wheel against 74.7 mm:

| term | today | 74.7 mm |
|---|---|---|
| torque | 1996 N m | 1700 N m |
| boost | 209 kPa | 208 kPa |
| T2 / charge | 483 / 355 K | 479 / 354 K |
| eta_c | 0.63 | 0.64 |
| tip Mach | 1.46 | 1.46 |
| phi/knee | 1.00 | 1.03 |
| shaft | 9231 rad/s | 10085 rad/s |
| vanes | 0.84 | **0.64** |
| p3/p2 | 1.22 | **1.32** |
| PMEP | 1.12 bar | 1.44 bar |
| air | 0.390 kg/s | **0.338 kg/s** |
| VE on the manifold | 0.609 | **0.543** |
| fuel (lambda 1.30, smoke-limited) | 23.2 g/s | 20.8 g/s |
| flags | 0.09 | 0.10 |

- **Measured:** it is not the over-speed relief (no over-speed bit). It is not the compressor either: the same
  ratio, efficiency and charge temperature. The smaller wheel asks more of the turbine, so the vanes shut further
  and p3/p2 rises from 1.22 to 1.32. The truck's VE then falls 11 %, and with the smoke limit the fuel falls with
  the air.
- **Suspected, not measured:** the truck's VE (0.61) is far under the tractor's (0.88) and far more sensitive to
  p3/p2 than Heywood's factor (1 % for that step). The truck build's own valve timing is the likely cause; that is
  D2's.
- **No shipped build shows it at rated:** the tractor gains at 1900 (2207 to 2256); the 2.0 loses 0.7 % at 4000;
  the pickup is unchanged.

## TurboTest:118 (nightly, "diesel turbo, full"): (b) alone turns it green

These are the case's three spools on the truck's sets at 1300, 4 s held, then 8 s at full rack. They come from
tee-calib-boost's take-up, which is close to the case's own.

| variant | big single | with vanes | compound |
|---|---|---|---|
| today | 28.3 kPa / 1042 N m | 31.5 / 1083 | 91.5 / 1169 |
| (b) | **62.8 / 1442** | **67.4 / 1363** | 91.5 / 1169 |
| (b) + the turbine's nozzle matched at the torque peak too | 49.8 / 708 (surge 0.51) | 49.7 / 707 | 91.5 / 1169 |

The case itself, run on the (b) tree, passes:

| spool | 90 % of | in | torque |
|---|---|---|---|
| single | 56 kPa | 7.30 s | 1375 N m |
| vanes | 60 kPa | 7.25 s | 1256 N m |
| compound | 83 kPa | 6.95 s | 1105 N m |

- **Moving the turbine's match point to the torque peak makes it worse.** The smaller nozzle raises the back
  pressure at 1300, the air falls and the compressor surges. So the proposal is not to move it: the turbine stays
  matched at rated.
- The compound is untouched by both, since compounds are excluded.
- The 8 s spool is still rising at its end (t90 over 7 s), which is the case's own window.

## Default tier on the (b) tree (guard on, no cap)

539 of 540 passed, in 11 min 26 s.

The red one is **diesel.brakes, BrakeTest.cpp:229**. It tests the engine, not the matching. The VGT engine brake on
the truck's sets at 2000 rpm, High, puts the shaft at most at 11480 rad/s against omegaMax 11124. The over-speed
relief acts at 1.05 x, which is 11680.

- **Measured:** the smaller wheel (74.7 mm) takes less power, so the ECU's shut vanes spin it 3 % past its limit.
  Braking still reads 388 kW retarding and 340 kPa manifold.
- **Reported, not adjusted.** It is D2's brake strategy against the smaller wheel. My suspicion is that a real VGT
  brake controller limits its vanes by turbo speed.

## Predictions (n4/predictions.md, written first), and how they fared

1. (b) guard on: the tractor as before, the truck at 1300 >= 2200, 1900 about 1700, the 2.0 as before, the pickup
   the same. **Right** (2160 from below, 2198 from above).
2. The cap: the tractor's 900 between, steady, binding only below the peak; the truck leaves its low state; the
   pickup at 1300 at 700-810. **Wrong throughout**: it latches at the tractor's 1000 and on the truck, and the
   pickup is unchanged.
3. The cap latches nowhere at or above a peak, else on the truck. **Wrong**: the tractor's 1000.
4. Take-ups within 0.5 s. **Wrong**: tractor +2 s, pickup +4 s.
5. Truck 1900: less air through a lower eta_c and a hotter charge, p3/p2 higher. **The air and p3/p2 right; the
   mechanism wrong**: eta_c and the charge are unchanged, and the cause is VE through the vanes.
6. Tier: TurboTest's truck cases pass, perhaps not :108. **They pass; I did not predict the brake's red.**
7. TurboTest:118: (b) alone single 35-60 kPa, green; the turbine at the peak makes the single > 100. **The first
   right (62.8); the second wrong (49.8)**.

Listen for (nobody here can hear):
- **The tractor at 1000 rpm on 76.8 mm:** a steady, strong turbo note. Under the cap it would hold at a weak hiss
  with a slow wander.
- **The truck's engine brake on High at 2000 rpm:** its turbo whine is about 3 % higher in pitch on the smaller
  wheel.
