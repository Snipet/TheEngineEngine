# The tractor's pull-away in one gear (27 Nov, the lead's ruling 3(b), 12:07)

The eighteen-speed takes the lever's slots, the clutch pedal and the range and splitter buttons (roadranger.fm); the
combo pull drove it with the lever in N, so it ran in neutral, governed (docs/measurements/combo/2026-11-26-shipped-pull.md,
"in neutral (no driver)"). Now, for a build whose "gearbox" is Eighteen-speed, the pull is a pull-away in one gear:
the clutch floored at 2.5 s (the clutch brake stops the input shaft at rest), the lever to a slot at 2.8 s, the clutch
let up from 3.4 s over a ramp, the throttle as every pull's, no shifts (the presses do nothing to it), the clutch down
and the lever to N at 13.5 s. The launch is unchanged for it. Its reference drives on its presses like every reference
(referenceScenario: on the reference's own manual, the truck's lever 5 would be fifth).

## The slot and the ramp, picked on 17 nudges (293.15 +- 0.15 K), the probe's GEARPROBE_DT

```
slot (ctl.lever)        ramp   stalls   top car.speed while driven   car.gear at the drive's end
5, L (gear 1)           1.0 s  0 / 17   2.34 to 2.36 m/s             1 in 17 of 17
1, low first (gear 3)   1.0 s  0 / 17   3.59 to 3.61 m/s             3 in 17 of 17
1, low first (gear 3)   2.0 s  0 / 17   3.57 to 3.60 m/s             3 in 17 of 17
```

Single draws first: the L slot at 0.5, 1.0 and 2.0 s, gear 1 at 2.96 s in each, governed at 210 rad/s, 2.1 m/s; the
low first at 1.0, 2.0 and 3.0 s, gear 3 at 2.96 s, 3.5 m/s. No stall at any choice (the third gear stalls on a dumped
clutch by B's trace; let up over a second it does not). PICKED: the L slot, 1.0 s: the lead's traced slot and the
lowest gear, the furthest from a stall; its top speed sits 0.34 m/s over the judge's "moves" (2.0 m/s) in every copy,
with a spread of 0.02.

## The case's rows, 17 copies against its every-switch-off reference's nightly bound

Reference's five copies 3.12 2.64 3.11 2.87 3.07, the bound 5.28 (the 26 Nov survey's to the digit: the same reference).

```
event       copies over / 17   largest excess
start       0                  1.47
cut         0                  1.81
limiter     0                  1.23   (the governor: the engine held at 210 rad/s in gear 1)
shift 1-0   0                  1.30   (the lever to N at 13.5 s)
the rest    0                  1.17
```

No row. The single draw at 293.15 K: "ok" (step 17.0, idle 848 rpm).

## Cost

The nightly case "a truck pulls away in its low slot: the tractor through the pull, in one gear": 27.2 s in its own
process with its reference played (12 s with it kept). Its reference is its own (no other case shares it), so a soak
pays about 27 s of a process here, about 7 s of wall at 4 in parallel: well inside the soak's line (+6 of 10 minutes
used). Committed with [soak].
