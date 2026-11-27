# The automatics and the CVT on a real pull: the lever in D (27 Nov, the lead's ruling 11:37)

The combo pull drove by the shift counters with the lever in N. An automatic and a CVT take R, N, D from the lever alone
(sim/vehicle/gearbox.fm:333), so on those boxes the pull ran in neutral (2026-11-26-shipped-pull.md, corrected). Now the
pull and the launch put the lever in D at 3.0 s and back in N as the car stops (13.5 s on the pull, 11.0 s on the
launch), for a build whose box is "Automatic" or "CVT" only (tests/sim/combo/Combo.cpp, steps and leverDrives); no other
box's lever moves (a manual's lever is its H-pattern: 1 is first). Held by "the pull drives an automatic and a CVT in D"
(fast-3), which fails with the lever left in N (gear 0, speed 0 at 8 s on both builds: shown).

## Prediction, written before the runs

- In N (before): car.gear 0 throughout on the automatic, the CVT and the eighteen-speed. HELD.
- In D: the automatic 1-2 and probably 2-3 under power; no cut asked (gearbox.fm: cut = false), so no X-047-family row,
  the shifts 0 of 17; the limiter not reached. The CVT in gear 1 throughout, no limiter. PARTLY HELD: the shifts and the
  CVT as said; but the hot-vee V8 made three upshifts under power (1-2 5.99 s, 2-3 7.04, 3-4 8.85) and four more on the
  lift (4-5 to 7-8), and three builds do reach a limiter (big-block, pickup diesel, V6 NA).

## car.gear, one draw at 293.15 K (the path is the same in every copy)

Hot-vee V8 (automatic): 0-1 3.38, 1-2 5.99, 2-3 7.04, 3-4 8.85, 4-5 10.68, 5-6 11.48, 6-7 12.28, 7-8 13.09, 8-0 13.88 (N);
39 m/s at 11 s. Economy four (CVT): 0-1 3.38, 1-0 13.88; 21.6 m/s at 11 s. Three-cylinder (manual, the control): 0-1 3.05,
1-2 6.55, 2-3 8.55, 3-2 11.05, the same to the digit as before the change.

## 17 copies at 293.15 +- 0.15 K, each against its own every-switch-off reference (tip, this commit's tree)

`BOUNDPROOF="corpus <stem> pull" BOUNDPROOF_DT=<the 17>`. Every event, every build: 0 of 17 over. (A window's count is
per copy; a shift is read both by the gear's change and by car.flags, so a shift has two windows a copy.)

```
build                          reference's five copies          bound   largest excess in any window, any copy
big-block-74-pushrod-v8        10.06 11.14 6.19 3.46 7.02        6.92    2.35 (limiter)
economy-four-15 (CVT)           8.98 8.61 9.28 8.51 8.67        17.02    1.93 (cut)
pickup-diesel-67-inline-six     1.28 1.92 1.21 1.68 2.04         2.41    2.04 (shift 4-3)
pushrod-v6-38                   6.00 3.33 1.69 1.50 1.68         3.01    2.33 (the rest)
twin-turbo-hot-vee-v8-40       12.14 2.39 10.95 9.08 9.43        4.77    3.67 (start)
v6-35-na                        7.62 7.67 8.62 5.64 9.66        11.29    2.39 (limiter)
v6-35-twin-turbo-pickup         8.79 8.27 3.36 8.87 9.96         6.71    3.57 (start)
```

No new row. Read with care: the every-switch-off references on the real pull carry excesses of 8 to 12 in most
copies, so most bounds are wide (the CVT's 17.02, the V6's 11.29): a 0 of 17 there is weak. The pickup diesel (2.41, its
own 2.04 at the 4-3) and the pushrod V6 (3.01) are the tight ones. Which event raises the references is not read here
(the corpus mode prints no reference windows): a question, not a finding.

Not measured: content/builds/diesel-pickup-tuned.json (Automatic) is outside the corpus (the archetypes only), in no case.

## What the rows judged until now

No combo case plays an automatic, a CVT or an eighteen-speed through the pull or the launch: the generator's six are
manuals (large-diesel by the default) and dual clutches; the four shipped pull builds are three manuals and a dual
clutch. The corpus plays every archetype through the tour (the CVT and the hot-vee V8 also in their own nightly cases),
and the tour moves no lever and presses no shift for any box: a free rev in neutral, as its name says. So the mislabel
was the survey's alone.
