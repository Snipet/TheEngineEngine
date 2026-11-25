# The VGT surge guard's opening gain at 0, with (b)'s wheels at the makers' speeds (25 Nov, local only)

The lead's ask (09:41): (b) at the makers' speeds with the vane surge guard's opening gain set to 0 (machine.fm,
`surgeOpen[t] + d16 * 1.0f`) in a local copy of sim/. Holds are 30 s from 800 rpm, reading the last 10 s means.
Cells give torque N m / boost kPa, the surge-flag mean (0 to 1; 4 = over-speed set), and phi/knee.

phi/knee is my reconstruction from outside, using the design ratio. It is not the model's own number.

The truck runs use GateTest's sets: model 1, single VGT, spring 220 kPa, icFlow 0.8, charge volume 0.02.
Predictions are in the scratchpad (predictions2.md) and are quoted below.

## Tractor, 76.8 mm

| speed | guard on (today) | guard off |
|---|---|---|
| 900 | 637 / 12, flag 0.08 | 845 / 58, flag 0.49, phi/knee 0.80 |
| 1000 | 775 / 36 pulsing 17-75 | 947 / 73 steady, 0.43, 0.88 |
| 1100 | 2722 / 251 | 2719 / 251 |
| 1300 | 2766 / 269 (choke flag) | 2768 / 269 |
| 1900 | 2255 / 222, over-speed | 2257 / 222, over-speed |

- Take-up to t90 at 1100: 3.5 s with the guard on, 4.0 s with it off.
- Pull 800 to 2000 at 25 rpm per 0.5 s, torque at 1100: 1821 with the guard, 2395 without.

## Truck, 76.2 mm: removing the guard brings back the gain-reversal state

| speed | guard on | guard off |
|---|---|---|
| 1000 | 297 / 12 | 180 / 12, vanes 0.30 |
| 1100 | 1020 / 20 | 393 / 19, vanes 0.30, flag 0.78 |
| 1300 | 2199 / 169 | **408 / 31**, vanes 0.32, flag 0.72, p3/p2 1.40 |
| 1900 | 1742 / 209 | 1443 / 192, vanes 0.62, flag 0.37 |

On the pull, guard off, boost stays under 111 kPa until 1800 rpm. With the guard, it reaches 170 kPa by 1600.
Without the guard the vanes shut to their stop and the compressor surges about 70 % of the time. The
back-pressure limit holds p3/p2 at 1.40 there, but it does not open the vanes enough to leave this state.
This is D2's 98 kPa / 882 N m second state again, and it is deeper here.

## 2.0 common rail and pickup

| build | speed | guard on | guard off |
|---|---|---|---|
| 2.0 today 39.1 mm, guard on, against (b) 36.9 mm, guard off | 1500 | 144 / 8 | 207 / 61 |
| same | 1750 | 155 / 14 | 253 / 91 |
| same | 2000 | 176 / 28 | 329 / 140 |
| same | 3000 | 331 / 169 | 330 / 165 |
| same | 4000 rated | 285 / 166, no flag | 282 / 171, **over-speed flag** (-1 % torque) |
| pickup 64.8 (unchanged) | 1300 | 576 / 21, vanes 1.00 | 810 / 77, vanes 0.61 |
| same | 1700 | 997 / 198 | 996 / 198 |
| same | 2800 | 941 / 240, over-speed | 941 / 240, over-speed |

## Default-tier cases with the guard gone

These ran with the guard-off machine.fm swapped in, then the file was restored. I ran tee-tests-diesel,
tee-tests-boost and tee-tests-calib-boost with ~[nightly].

- **TurboTest:88 and :108 fail.** These test the engine: a VGT on the truck at 1300 holds a fixed single's boost.
  It reads 23.0 kPa against the single's 27.9.
- **TurboTest's one-state case** and **GateTest's short-form one-state case** pass. Both read one state, but it
  is the low one: 28 kPa / 396 N m from 1100 and from 2000, against about 75 kPa today.
- **LayoutTest:183 tests the guard's own behaviour.** It passes vacuously (surgeOpen all 0). It goes with the guard.
- The two failures already marked [!mayfail] are unchanged in kind: LayoutTest:178 and GateTest:129.

## The tractor's 1000 cell, guard off: the turbo's ledger

These are 1 s exponential means at the sample rate, from an instrumented local copy.

| term | value |
|---|---|
| shaft power | turbine 19.9 kW, compressor 19.6 kW, bearings 0.2 kW |
| efficiency | eta_t 0.66 (mass-weighted), eta_c 0.57 instantaneous / 0.54 from power |
| vanes | 0.63 |
| mass flow | turbine 0.207 kg/s, compressor 0.208 kg/s, gate 0 |
| pressures | p3 244 kPa, p4 106 kPa, p2 175 kPa |
| T3 | **673 K** |

The isentropic compressor work is 10.7 kW. The turbine has 30.1 kW available, so Pt over that is 0.66.

With the lead's terms (T3 900 K, eta_t 0.60, eta_c 0.65), p3/p4 1.73 would do, which is p3/p2 about 1.05.
With the model's terms it needs 2.18, and it runs at 2.30.

The short term is **T3**: 673 against 900, a factor of 0.75. Next is eta_c: 0.57 against 0.65, a factor of 0.87.
Together they come to 0.65, which is the lead's two thirds. eta_t is not short.

Measured: T3 is 673 K at lambda 1.30, full load, 1000 rpm. Suspected: that is low for a heavy diesel there. It
is the exhaust side's temperature (D's and G's), not the turbo's.

## The rule's flow law against the engine at that cell (in the model's own terms)

These are the model's own values, 1 s means from the instrumented copy, at 947 N m / 74 kPa. They are set beside
the rule's evaluation at the same speed and at the PR the engine reaches, 1.75.

| term | model | rule |
|---|---|---|
| tip Mach | 1.005 | 0.986 |
| knee phiK | 0.4005 | 0.3975 |
| choke flow mc | 0.562 kg/s | 0.567 kg/s |
| fresh air | 0.208 kg/s (compressor), 0.211 kg/s (by lambda) | 0.224 kg/s |
| phi/phiK | **0.877** (the mean of the instantaneous phi, 0.352) | **0.995** |

The gap is a product, 0.928 x 0.951 x about 1.00 = 0.884:

- **Fresh-air VE: 0.836 against the rule's 0.9 (factor 0.928).** It comes from the compressor's flow at the
  manifold's 175 kPa and 305 K.
  - engine.veManifold reads 0.895, but it is not the fresh charge. My first reading, which said "VE matches", was
    wrong.
  - Suspected: residuals and backflow at p3/p2 1.40.
  - The cooler charge, 305 K against 310 K, is inside this factor and helps by about 1.6 %.
- **The pulse (factor 0.951).** The flag reads the instantaneous phi. Its mean is 0.352, against 0.370 as a ratio
  of means.
- **The map: within 1 %.**

So the rule's 10 % margin is used up by VE and the pulse.

The lead's ruling (10:01): when (b) is written into the matching, its margin is stated in the model's own terms.
That means the instantaneous flow (a pulse allowance from the cylinder count and the speed) and the fresh-air VE
under the build's back pressure. It is not sized until D2's cylinder ledger at this cell is in.

At the cell, from the cylinder side: 224 mg a stroke (smoke-limited, lambda 1.30), an exhaust port at 756 K, T3 at
673 K. Indicated efficiency is 26 %, brake efficiency 20.7 % (LHV 42.8 MJ/kg). These went to D2.

## Predictions, and how they fared

1. Tractor: 900 near 40-50 kPa, 1000 at 73, the rest within 2 %, take-up no slower.
   **Right except take-up**: 900 read 58 kPa, and take-up was 0.5 s slower.
2. Truck like the tractor. **Wrong**: it falls into the gain-reversal state from 1000 to 1900.
3. 2.0: 2000 up 50 %, 1500 and 1750 within 5 %, rated within 2 %, over-speed flag at 4000.
   **2000 +87 %; 1500/1750 wrong** (+44 % and +63 %); **rated and the flag right**.
4. Pickup: the guard does nothing. **Wrong**: at 1300 the guard pins the vanes open, 576 against 810 N m.
5. The ledger's short term is eta_t. **Wrong**: it is T3; eta_t is 0.66.
6. The flow-law gap is mostly VE. **Right after all**: fresh-air VE (0.836) is the larger factor. My first reading of
   veManifold 0.896 as the VE was wrong.
7. The two named cases still pass. **Wrong**: TurboTest:88 and :108 fail. The one-state cases pass in the low state.

Listen for (nobody here can hear): the truck at 1300 rpm full load with the guard off. Expect a turbo with no
whistle, a surging chuff at the inlet, and a flat heavy note. The tractor at 900 to 1000, guard off, should hold
a steady low boost hiss instead of today's 1 Hz cycle.
