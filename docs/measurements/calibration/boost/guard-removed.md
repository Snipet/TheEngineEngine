# The guard removed, on (b): the shipped machine diesels, and the two things seen on the tractor-valved truck
(25 Nov, local; n2-b-local cff355f9 "no vane surge guard", over (b))

## The commit

- **machine.fm:** the guard's opening path is gone. The surgeOpen state, the integral, and its use on the vanes are
  removed, not zeroed, with a comment on why.
- **The surge flag** stays as a reading (U3's oversize warning).
- **LayoutTest's case on the guard's own behaviour** goes with it.
- Lint ok. The layout tests pass (one marked case failing as expected, as before).

## The shipped machine diesels, guard on ((b) as it was) against guard off

Holds are 30 s, read as the last 10 s means. Cells give N m / kPa (the 1 s means' range), the vanes, and the
surge-flag mean. "Up" is from below; "down" is from above (the tractor from 1900, the pickup 2800, the 2.0 4000,
the large diesel 2500).

| build, direction | guard on | guard off |
|---|---|---|
| tractor, up: 900 / 1000 / 1100 | 1403 / 31 (v1.00 f0.50); 2579 / 225; 2720 / 251 | **2080 / 106 (103-106)**, v0.65; 2578 / 226; 2718 / 251 |
| tractor, down | 1410 / 32; 2582 / 224; 2718 / 252 | **2099 / 107**; 2582 / 224; 2716 / 252 |
| pickup, up: 1500 / 1700 / 1900 | 698 / 44 (v1.00); 997 / 198; 1015 / 200 | **955 / 111 (107-112)**, v0.71; 997 / 198; 1015 / 200 |
| pickup, down | 705 / 45; 997 / 200; 1017 / 200 | **959 / 112**; 997 / 200; 1017 / 200 |
| 2.0, up: 1500 / 1750 / 2000 | 151 / 14; 158 / 16 (v1.00); 329 / 140 | **207 / 61 (58-62)**; **253 / 91 (88-92)**; 329 / 140 |
| 2.0, down | 151 / 14; 159 / 17; 329 / 140 | **207 / 61**; **256 / 93**; 329 / 140 |
| combo large diesel (twin wastegates), up and down: 1300 / 1520 / 1700 | 781 / 56; 1460 / 57; 1392 / 54 | identical |

### Measured

- **No shipped build gains a low state.** From below and from above agree within 1 % at every cell, guard off.
- **Guard off is at least guard on everywhere.** The gains:
  - the tractor's 900: +48 %;
  - the pickup's 1500: +37 %;
  - the 2.0's 1500 and 1750: +37 % and +60 %.

  The peaks and the speeds above them are unchanged within 0.2 %.
- **No cell in a shipped build swings more than 5 %.** The largest range is the pickup's 1900, 200-211 kPa, the same
  with the guard.

### Noted, pre-existing, not judged here

- The combo large diesel sits at 54-57 kPa with its flags at 1 / 3 / 7 (surge, choke, over-speed) from 1300 to
  1700, the same with or without the guard. It has wastegates, so no vanes, and (b) leaves its wheel unchanged.

## The tractor-valved truck (D2's fixture in a scratch copy, the tractor's valve events), guard off

### The swing at 1000 and 1100: the compressor's own deep surge, about every 2 s

A 0.25 s trace at 1000 shows a repeating cycle:

- The boost climbs from about 170 to 210 kPa with the vanes following the spring law (0.57 to 0.65), and p3/p2 at
  the 1.4 limit.
- At about 208 kPa it crosses the flag, and the surge flag reads 0.43 to 0.57 for one window.
- The boost drops to 162-175 kPa in under 0.25 s and the shaft falls 9 % (10100 to 9500 rad/s). Then it climbs again.
- Torque holds at 2450-2550 N m through it: lambda 1.44-1.50, so the fuel curve limits, not the air.

**Measured:**
- It is not the vanes' rate: they follow the boost.
- It is not the back-pressure limit's integral: p3/p2 stays 1.3-1.45 across the cycle.
- It is a surge-and-recover limit cycle. The truck's sets ask for 220 kPa by the spring, with no target, and at
  1000 rpm the compressor reaches its flag line at about 208 kPa.

**What differs from the shipped tractor at 1000** (steady at 225 kPa):
- The tractor's loop holds a 260 kPa target, but the back-pressure limit stops it at 225 with phi/knee 1.03, right
  of the flag.
- The truck's 74.7 mm wheel at 1000 sits at phi/knee 0.99 on the mean (my outside estimate). That is only 10 % right
  of the flag, and the climb toward the spring crosses it.

**Suspected, not measured:** the rule promised 10 % at the design ratio. The truck's sets run a hotter charge than
the rule assumes: the rule reads icEffect, and the sets change icFlow. That would cut the margin at the top of the
climb.

### The choke flag at 1900

At 1900 the flag is mostly choke (2) with the vanes fully open at 200 kPa. (b) chose the torque-peak wheel over F's
1.15 at rated, so at rated the smaller wheel passes its choke.

The same kind of sign shows on the shipped builds:
- the tractor's choke flag at 1300 on 76.8 (a mean of 1.8 to 2.3);
- the 2.0's over-speed flag at 4000;
- the pickup unchanged.

Torque at the truck's 1900 is 2128 against 1705 on its own valves. It costs no torque that I measured. It is the
rated end's margin traded for the torque peak's, and that trade is the lead's to rule.

## Predictions (n7/predictions.md, written first)

1. No shipped low state, up and down within 5 %. **Right** (within 1 %).
2. Guard off at least guard on, with gains at the tractor's 900 (about 2000), the pickup's 1500, and the 2.0's
   1500/1750 (about 200/250). **Right** (2080; 955; 207/253).
3. The twin wastegate builds unchanged to the newton metre. **Right.**
4. A 10-20 % swing somewhere among the shipped builds. **Wrong**: none over 5 %. The truck fixture's swing is a
   surge cycle of its own.

Listen for (nobody here can hear):
- **The tractor at 900 rpm full load, guard gone:** a boosted note at 106 kPa where today it is flat.
- **The tractor-valved truck at 1000:** a deep surge "whoomp" every 2 s with the turbo's pitch dipping. That is
  what a compressor at its surge line sounds like.

## The icFlow point (the lead's 12:54), measured

### The premise did not hold

- icFlow sets only the intercooler's pressure drop (icK = icDrop / icFlow^2, machine.fm).
- The charge's temperature is icEffect against the core, and the engine dyno holds the core at the air's
  temperature (SAE J1349, testCell). The rule reads icEffect, which is right.
- The truck's sets state neither icEffect nor icDrop, so they take the defaults.
- The shipped builds state both: icEffect 0.75 on all three, and icFlow 1.09 / 0.578 / 0.204.

### What the rule did leave out, measured inside the model at the truck-valved fixture's 1000 cell

| term | model | the rule's assumption |
|---|---|---|
| compressor efficiency | eta_c 0.64 | compEta, the map's peak |
| T2 | 485 K | |
| charge | 350 K | |
| the intercooler's pressure drop | | ignored |

### The correction (local commit ed48581f, in (b))

- The efficiency is the map's own at the margin's point (0.99 of the knee).
- The cooler's drop at the engine's whole flow, icDrop (m / icFlow)^2, is subtracted from the boost.
- The wheels move about 1 %:
  - tractor 76.8 -> 76.2 mm;
  - the truck's sets 74.7 -> 73.9;
  - the 2.0 36.9 -> 36.6;
  - the pickup and the combo diesel unchanged.
- The tractor's 1000 stays 2581 N m / 228 kPa. Its 1300 choke flag reads 2.38, against 1.8 to 2.3.

### The 2 s cycle survives the correction, and it is not the margin

On the corrected rule the cycle survives: 163-204 kPa at 1000, 141-195 at 1100.

The same fixture held to a target by the boost loop, in place of its spring alone, is steady:

| target | boost | PR | phi/knee | surge flag | torque |
|---|---|---|---|---|---|
| 180 | 182 (179-187) | 2.88 | 1.03 | 0 | 2581 |
| 195 | 195 (195-196) | 2.99 | 1.04 | 0 | 2561 |
| 205 | 205 (198-213) | 3.09 | 1.04 | 0 | 2539 |
| 215 | 221 (220-222) | 3.24 | 1.05 | 0 | 2501 |

- **Measured:** the rule's margin holds on the steady climb (phi/knee 1.03 to 1.05 against its 0.99).
- **Measured:** the cycle belongs to the vanes' spring law (no target: the vanes opened in proportion to the signal
  over 0.8 of the spring), not to the wheel.
- **Suspected, not measured:** a proportional law with the vanes' 0.05 s lag and the shaft's inertia makes a limit
  cycle, and its peak crosses the flag. The truck's sets state no target; the shipped VGT diesels all state one.

### Predictions (n8/predictions.md)

1. The wheels shrink 1.5-3 %. **Wrong in size**: 0.8-1.1 %.
2. The cycle weakens but survives. **It survives; it did not weaken** (163-204 against 165-210).
3. The tractor's 1000 within 1 %, its 1300 choke flag a little more often. **Right.**
