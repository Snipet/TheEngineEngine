# The turbine nozzle's inlet factor: a sensitivity on the 15 L tractor (25 Nov, local only)

The lead's ask (09:13): the design factor p3 = 1.247 x PR x pOut (machine.fm) against 1.10, 1.00 and 0.95, on the
76.8 and 80.6 mm wheels; measured, nothing changed. Each cell: a 30 s hold from 800 rpm, the last 10 s means
(tee-calib-boost via a hold script), torque N m / boost kPa (the 1 s means' range over the last 21 s), p3/p2 (absolute),
BSFC g/kWh. Take-up: P's command, the throttle opened at 1100 rpm, t90 of the boost at 0.5 s resolution.

**EGR:** none is modelled anywhere in sim/ (no field, no path); the X15's real p3/p2 above 1 at its EGR points has no
counterpart here. The back-pressure limit (spec boost.backPressure, 1.4, judgement) is what holds p3/p2 at 1.40.

| wheel | factor | 1000 | 1100 | 1300 | 1900 | t90 @1100 |
|---|---|---|---|---|---|---|
| 76.8 | 1.247 | 775 / 36 (17-75) 1.20 399 | 2722 / 251 (244-252) 1.40 206 | 2766 / 269 (265-271) 1.38 203 | 2255 / 222 1.60 190 | 3.5 s |
| 76.8 | 1.10 | 718 / 29 (11-71) 1.24 407 | 2720 / 245 (239-246) 1.40 206 | 2762 / 265 1.40 203 | 2300 / 220 1.49 186 | 3.5 s |
| 76.8 | 1.00 | 693 / 24 (9-47) 1.22 407 | 2718 / 241 (235-242) 1.40 206 | 2763 / 261 1.40 203 | 2333 / 220 1.41 185 | 4.0 s |
| 76.8 | 0.95 | 681 / 22 (7-48) 1.22 409 | 2722 / 239 (233-240) 1.40 206 | 2764 / 259 1.40 203 | 2337 / 219 1.40 183 | 4.0 s |
| 80.6 | 1.247 | 657 / 14 (14-14) 1.11 398 | 2720 / 237 (232-237) 1.40 206 | 2757 / 265 1.40 203 | 2230 / 243 1.63 191 | 4.0 s |
| 80.6 | 1.10 | 644 / 12 (11-14) 1.11 400 | 2719 / 231 1.40 206 | 2764 / 259 1.40 203 | 2280 / 242 1.51 187 | 4.0 s |
| 80.6 | 1.00 | 638 / 11 (10-14) 1.11 404 | 2720 / 227 1.40 206 | 2763 / 255 1.40 203 | 2319 / 238 1.42 184 | 4.5 s |
| 80.6 | 0.95 | 641 / 11 (9-14) 1.11 402 | 2718 / 225 1.40 206 | 2763 / 253 1.40 203 | 2332 / 234 1.40 184 | 5.5 s |

The over-speed flag stands at 1900 in every row; 1300 on 76.8 at 1.247 also flags choke (2.09 mean).

## What 1000 rpm is short of (a diagnostic, local copies of sim/, 76.8 mm, factor 1.247)

| case | 1000 rpm |
|---|---|
| today | 775 N m, 36 kPa, pulsing 17-75 |
| surge guard's opening gain 0 | 947 N m, 73 kPa steady (72-74), p3/p2 1.40, phi/knee 0.88 |
| same, 80.6 mm | 873 N m, 61 kPa steady |
| backPressure 1.8, guard on | 1024 N m, 100 kPa, pulsing 18-153 |
| backPressure 1.8, guard off | 1044 N m, 113 kPa steady, p3/p2 1.79, phi/knee 0.86 |

Measured: the 1 Hz pulse is the vane surge guard's open-and-give-back cycle (gain 0: steady at once, both wheels);
the nozzle factor does not touch it. With the guard off, the back-pressure limit holds the vanes and boost sits at
73 kPa; the fuel is smoke-limited (lambda 1.30) at every 1000 cell, so the engine's torque there follows its air.
Suspected, not measured: the real engine's plateau from 950 rests on vanes run past 1.4 for EGR and on a vane map
with its own surge margin; neither is in the model.

## Predictions (written before the runs, scratchpad), and how they fared

1. 1900: p3/p2 falls with the factor; BSFC 2-4 % better; the over-speed flag clears at 1.00 or below. **Right on the
   first two** (1.60 to 1.40, the limit's floor; 190 to 183, -3.7 %); **wrong on the third**: it stands in every row.
2. 1100 holds at 1.10/1.00 as the vanes close; at 0.95 the vanes saturate and boost sags 10-20 %. **Wrong in
   mechanism and size**: the vanes are on the back-pressure limit (1.40) in every row, not saturated; boost sags 5 %.
3. 1000's pulse is not cured by a larger nozzle; likely worse. **Right** (36 to 22 kPa on 76.8).
4. Take-up slower as the factor falls, +10-30 % at 0.95. **Right on 76.8** (+14 %); **too small on 80.6** (+38 %).
5. 80.6 holds a steady hole at 1000 at every factor. **Right.**
6. Guard off: 1000 on 76.8 steady above 100 kPa. **Steady right, level wrong**: 73 kPa, the back-pressure limit.
7. backPressure 1.8: guard off higher and deeper in surge, guard on still pulsing. **Right on both**, though
   phi/knee rose (0.86) rather than fell.

Listen for (nobody here can hear): the tractor at 1000 rpm full load, today against guard-off: a 1 Hz whoosh-and-sag
of the turbo against a steady note; at 1900, the turbine's hiss a touch lower with the factor at 1.00.
