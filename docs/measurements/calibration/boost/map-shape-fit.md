# The compressor map's shape fitted to every cited map: stage one, the fit alone (25 Nov, local; no engine run)

The lead's ruling (14:00): fit the map's three shape laws, and the choke flow's scale, to every cited map the model
has been read against. The forms were stated before any number. The fit is in map-shape-fit.py beside this file. It
uses Nelder-Mead on the residuals divided by each reading's stated precision, with three starts.

## The forms

| law | form | today |
|---|---|---|
| rise | tau_knee = c(M) 0.4 M^2, c(M) = c0 + c1 (M - 1) | c = 0.45 |
| knee (the surge end) | phiK = (k0 + k1 (max(M,0.8) - 1)) kAr | 0.40, 0.178 |
| fall | tau at choke = f tau_knee, the ellipse from the knee kept | f = 0.35 |
| choke scale | k rho0 a0 A_ind arWide (0.3 + 0.7 min(M/1.63, 1)) | k = 0.45 |

What the fit does not change, and what it assumes:
- **The efficiency law is the model's, unchanged.** The maps' stated peaks are used.
- **A drawn line's right end** is where that law falls to the contour P names: the S400SX-E 0.59 (0.54 to 0.64), the
  S300SX3 0.64 (0.62 to 0.66).
- **A passenger map's "choke at the last contour"** is the most flow at that ratio among the drawn lines with
  efficiency 0.58 or more. 0.58 is Garrett's stated definition; for the EFR and the S400SX3 it is assumed.
- **Flows are corrected at 293 K.** The maps do not state their reference.

## The data

- **P's two heavy-duty maps, line by line** (sources section 21, 7ad4c0bc): 14 lines, each with its peak ratio, surge
  end, right-end flow and right-end ratio.
- **The five maps of published-compressor-maps.md, ratio by ratio:** surge, and choke at the last contour. These are
  the maps the present knee law was fitted to: the GT2860RS, the GTX2860R Gen II, the GT2560R, the EFR 6258, and the
  S400SX3, which is doubtful at 2 to 4 lb/min.

## The fits

RMS is in reading precisions.

| fit on | c0 | c1 | k0 | k1 | f | k | RMS (n) |
|---|---|---|---|---|---|---|---|
| **all seven maps** | **0.608** | **-0.157** | **0.349** | **+0.551** | **0.652** | **0.506** | **6.07 (90)** |
| P's two (heavy-duty) | 0.606 | -0.155 | 0.340 | +0.555 | 0.653 | 0.515 | 6.91 (56) |
| the knee law's five | 0.629 | -0.216 | 0.396 | +0.449 | 0.491 | 0.465 | 2.79 (34) |
| the S400SX-E alone | 0.597 | -0.151 | 0.334 | +0.558 | 0.603 | 0.525 | 5.48 (36) |
| the S300SX3 alone | 0.581 | -0.077 | 0.385 | +0.501 | 0.754 | 0.462 | 3.14 (20) |
| **today's model** | 0.45 | 0 | 0.40 | 0.178 | 0.35 | 0.45 | **17.55** (P's two 21.49, the five 7.43) |

### Hold-out

- The S400SX-E's fit on the S300SX3: RMS **10.25**, against 3.14 for the S300SX3's own fit.
- The S300SX3's fit on the S400SX-E: **14.35**, against 5.48 for its own.

### Cross-class

- P's two maps' fit on the five: **4.57**, against 2.79 for the five's own fit.
- The five's fit on P's two: **12.04**, against 6.91 for their own.

## Reading

### Measured

- **All three laws move the same way on every fit.**
  - c falls with Mach: about 0.60 at M 1 to about 0.52 at M 1.63. That is above today's 0.45 everywhere.
  - The knee rises about three times as fast with Mach as today's law (k1 0.45 to 0.56 against 0.178). At M 1.5,
    phiK is 0.62 against 0.49: the surge end of a fast line sits a quarter further right.
  - A line falls only to 0.49 to 0.75 of its knee's rise at its right end, against 0.35 today.
- **The all-maps fit cuts the error by two thirds** (17.55 to 6.07).
- **The fit is not within reading precision (RMS 3 to 7), and the hold-out shows it.** The two heavy-duty maps disagree
  with each other beyond their precision, mostly in f (0.60 against 0.75) and k (0.525 against 0.462). The S300SX3's
  lines hold their ratio further toward the right end: at 561 m/s it ends at PR 3.64, the S400SX-E at 2.69.
- **The largest residuals are the right ends** (ratio and flow at 230-280 m/s, and the S300SX3's 561 m/s end). There
  the fixed efficiency law places the end. It is not in the fit, and its contour assignment is P's range, not a
  per-line reading.
- **The 607 m/s line** (S400SX-E, past the model's 560 m/s tip limit): the linear c gives PR 5.29 against 4.99. That
  is the bend. It binds nothing the model runs, since the shaft stops at 560.

### What the sheets state that could tell the maps apart

| map | stated |
|---|---|
| S400SX-E | trim 56.3 |
| S300SX3 | trim 52.3 |
| GTX2860R | a ported shroud |
| S400SX3 | recirculation grooves (a map-width feature) |
| GT2560R | A/R 0.80 |
| GT2860RS | A/R 0.60 |
| EFR 6258 | A/R 0.64 |

The two heavy-duty maps' disagreement (f, k) goes with trim, 56.3 against 52.3. That is one pair, so it is not a law.

A build can state a wheel's trim (inducer over exducer, squared): the model has compInducer and compExducer. Suspected,
not fitted: trim as a second parameter for f or k. It needs more than two maps to be more than a line through two
points. Not entered.

## Proposal for stage two (on the lead's word)

1. The all-seven fit's laws, which are within each sub-fit's spread for c and the knee:
   - c(M) = 0.608 - 0.157 (M - 1);
   - phiK = (0.349 + 0.551 (M - 1)) kAr;
   - f = 0.652;
   - k = 0.506.

   Every constant cites this file and section 21. The surge flag stays at 0.9 of the knee.
2. Then re-read (b)'s wheels, the tractor's both ends, the 2.0, and every shipped boosted build's validation rows.

## Predictions for stage two, written now

1. **(b)'s wheels shrink about 8-12 % in inducer.**
   - The climb's binding ratio is reached at lower Mach (c higher), where the knee is lower. But the new knee law is
     higher there: at M 1.4 it is 0.57 against 0.47. And k is +12 %.
   - Net: area about x0.78. The tractor comes to about 67-70 mm, the truck's sets about 65-68, the 2.0 about 32-34.
   - The surge margin at the torque peak comes out as the rule states it. The point of the change is that the margin
     is now on a map that matches the published ones.
2. **The tractor's 1000 is held within 3 %** by the rule's own construction.
3. **At rated a flat 260 kPa sits under the limit** (c 0.51 at M 1.63 reaches PR 4.6 at the knee; f 0.65 holds the
   ratio toward the right end), at a shaft of about 0.90. (A) is no longer needed.
4. **The 2.0's 1750 within 5 %, its 4000 under 1.0.**
5. **Petrol boosted builds:**
   - More ratio at the same shaft speed and a further-right surge line on every wheel.
   - Builds with a stated wheel (compInducer) see their surge margin shrink at low speed. Expect new surge flags at
     low speed on one or two of them (the turbo four, the 1.0 three).
   - Matched wheels shrink and spool sooner.
   - Misses among the boosted rows change by 2 to 4, in both directions.
