# Flow sheets against the valve's low-lift area: the seat's frustum tried and refuted

Card N1, 25 Nov, 02:13 UTC (by `date -u`). The lead's check before a push. The sheets are P's,
docs/measurements/2026-11-25-0032-judgement-sources.md §12.

## The question

A trial law (350b86be, never pushed) made a valve's area at low lift its seat's own passage, not 0.73 of the curtain on
the head. The passage was the frustum normal to a single 45° seat face of width w (3 % of d intake, 4.5 % exhaust),
from Heywood's geometry as recalled, and it carried a coefficient of 0.8 (judgement). It lowered the warm idle's burned
share on the big-cam engines by 0.04 to 0.06 (V10 0.399 → 0.347). Before it went in, three outcomes were set out:

- (a) the sheets' low points sit near 0.8 of the frustum;
- (b) they sit near the frustum or the 0.73 curtain;
- (c) a point exceeds the frustum.

## Method

Each sheet point becomes an effective area: at 28 in of water a square inch passes about 147 cfm at a coefficient of
one, and every sheet below states 28 in. Each area is set against two things at that lift:

- the curtain on the head's diameter, π d L;
- the frustum with w as above.

The arithmetic on the sheets' figures is **measured**. The sheets themselves are third party, with their caveats.

## The sheets, as area over curtain | area over frustum

LS7 production head (GM CNC-ported; RLD Performance; corrected; valves 2.20 / 1.61 in; seat w 1.7 / 1.8 mm assumed):

| lift | intake | exhaust |
|---|---|---|
| 0.100 in | 0.70 \| 1.03 | 0.67 \| 1.01 |
| 0.200 in | 0.71 \| 0.99 | 0.81 \| 1.15 |
| 0.300 in | 0.73 \| 0.93 | 0.71 \| 0.93 |
| 0.400 in | 0.67 \| 0.81 | 0.65 \| 0.81 |
| 0.500 in | 0.62 \| 0.73 | 0.56 \| 0.67 |

882 small block (1.94 / 1.50 in; a forum member's figures, a search snippet):

- 0.100 in: intake 0.78 | 1.14, exhaust 0.84 | 1.26.
- 0.200 in: intake 0.70 | 0.94, exhaust 0.78 | 1.10.

K20A2 intake (a snippet; no valve sizes given, so 2 × 35 mm from memory):

- 0.100 in: 0.85 | 1.22.
- 0.200 in: 0.77 | 0.97.

## Reading: outcome (c)

- At 0.100 to 0.200 in, every sheet reaches 1.0 to 1.26 of the frustum. That is a coefficient of one or more on the
  passage, which is impossible.
- A single 45° face, even with its width taken to zero, cannot pass more than cos 45 ≈ 0.707 of the curtain. The sheets
  reach 0.70 to 0.85.
- No choice of seat width saves it. The LS7's exhaust at 0.200 in fails at any w.
- So the passage a real seat offers is larger than the single-angle frustum. Multi-angle valve jobs, back-cut valves
  and a top cut are the suspects (judgement, not a source).

The law stays as it was (68e683f1): min(q A0, 0.73 curtain).

- The 0.73 of the curtain sits on the LS7's sheet within 0.03 from 0.100 to 0.300 in.
- It sits under the other two sheets at 0.100 in (0.78 to 0.85). It under-reads them rather than over-reads.
- The trial's idle shares are withdrawn with it. N3-16's gap is not the low-lift area as far as a sheet reaches; see
  the lead's reading on the cards.

**What no sheet reaches.** Under 0.100 in (2.5 mm) nothing is measured, and all of the overlap's flow passes there (0.3
to 2 mm). The law there is the curtain's slope carried down from where the sheets begin, marked as that. A measured
coefficient below 2.5 mm, either direction, replaces it; reverse-flow-sizing.md prices one in reverse.

The trial's coefficient of 0.8 at about 1 mm (judgement, from a short channel's losses) is not contradicted by itself.
The geometry it multiplied is what fails.

## Side finding: the port limit above a quarter of the diameter (measured, nothing changed)

The port's limit is n q A0, with A0 = 0.4 (π d² / 4) tanh(6.8 L / d) / tanh(1.7).

K20A2 intake against the two-stage build's head (2 × 34 mm, q 1.65), cfm at 28 in:

| lift | sheet | model's limit | sheet / limit |
|---|---|---|---|
| 0.100 in | 107.8 | 136.8 | 0.79 |
| 0.200 in | 196.5 | 224.4 | 0.88 |
| 0.300 in | 261.7 | 265.6 | 0.99 |
| 0.400 in | 294.7 | 282.2 | 1.04 |
| 0.500 in | 307.8 | 288.5 | 1.07 |
| 0.600 in | 316.6 | 290.8 | 1.09 |

LS7 sheet over the 7.0's limit (56 / 41 mm; q 1.35 / 1.5), at 0.100 to 0.700 in:

- intake: 0.73, 0.81, 0.94, 0.99, 1.06, 1.10, 1.13, 1.12, 1.12;
- exhaust: 0.65, 0.90, 0.96, 1.07, 1.10, 1.13, 1.15, 1.15, 1.15.

From 0.300 in to its last point, the K20A2 sheet climbs 21 % and the model 9.5 %. From d / 4 to the last point, the
sheets rise 15 % (K20A2, LS7 exhaust) and 6 % (LS7 intake, whose d / 4 is 0.55 in), against the tanh's 6.3 % and 4.1 %.
The tanh goes flat where the sheets still climb.

On the K20A2, from the two-stage build's stage A lift (8.5 mm) to stage B (12.65 mm) the sheet rises 12.6 %
(interpolated); the model gives 5.6 %. Three sheets, one a snippet, give a shape, not a law, and nothing is changed
here. The form of the limit is the lead's to rule. The 7.0's 6 % shortfall at high lift is content (its port quality,
P's).

## A second form tried and refuted: valve and port in series (the lead's proposal, 02:12)

The form tested was the curtain at its coefficient (Ac = n 0.73 pi d L) and the port's throat (Ap) as two
restrictions in series: A = Ac Ap / sqrt(Ac^2 + Ap^2). It adds no new constant. Each head's Ap was read from its own
sheet's top, so the form passes through the last point. For fairness, the tip's min(q A0, Ac) was anchored the same
way, with q through the last point. The K20A2's valves are 2 x 35 mm from memory.

Model over sheet, as series | tanh-min:

| lift | LS7 in | LS7 ex | K20A2 in | 882 in | 882 ex |
|---|---|---|---|---|---|
| 0.10 in | 1.03 \| 1.04 | 1.06 \| 1.09 | 0.84 \| 0.86 | 0.90 \| 0.93 | 0.83 \| 0.87 |
| 0.20 in | 0.98 \| 1.02 | 0.84 \| 0.90 | 0.85 \| 0.95 | 0.92 \| 1.03 | 0.79 \| 0.94 |
| 0.30 in | 0.91 \| 1.00 | 0.88 \| 1.03 | 0.86 \| 1.07 | 0.87 \| 0.95 | 0.82 \| 0.93 |
| 0.40 in | 0.93 \| 1.09 | 0.88 \| 1.08 | 0.91 \| 1.04 | 0.87 \| 0.92 | 0.89 \| 0.96 |
| 0.50 in | 0.93 \| 1.05 | 0.93 \| 1.04 | 0.96 \| 1.02 | 0.95 \| 0.97 | 0.96 \| 0.99 |
| 0.60 in | 0.94 \| 0.99 | 0.95 \| 1.00 | 1.00 \| 1.00 | 1.00 \| 1.00 | 1.00 \| 1.00 |

The port areas read from the tops, as a share of the valves' area: LS7 0.86 / 0.91, K20A2 0.88, 882 0.56 / 0.62.

**Refuted:** the series form under-reads every column through mid-lift, by 7 to 21 % at 0.2 to 0.4 in. A head
saturates more sharply than two equal restrictions in series. The tanh-min, anchored the same way, stays within -8 to
+9 % from 0.2 in up. The sheets' shape lies between the two. A blend (a power mean with its exponent) would be a
constant fitted to three heads, so it is not proposed.

**No change to the area law.** The question waits for further production four-valve sheets with stated valve sizes
and depression (asked of P). If they agree on a shape, it is brought then.

The 7.0's port quality from the LS7 sheet, under today's law (content, P's): least squares on the relative error over
0.300 to 0.600 in gives portIn 1.44 and portEx 1.68. The model over the sheet at 0.300 / 0.400 / 0.500 / 0.550 /
0.600 in:

- intake: +0.2 / +7.5 / 0.0 / -2.8 / -5.6 %;
- exhaust: +3.1 / +4.3 / +0.6 / -1.7 / -3.3 %.

## Ten production four-valve heads (P's sheets, 9594f39c)

The heads are nine rows from Stan Weiss's compilation, at 28 in with their valves stated (a secondary compilation,
"use at your own risk"), and DSport's FA20D, whose depression is not stated, so it counts for shape only. Each form is
anchored at the sheet's last point as above. The arithmetic is **measured**.

The sheet's rise from d / 4 to its last point, intake / exhaust:

| head | rise |
|---|---|
| K20A2 | +14 / +14 % |
| 2ZZ-GE | +14 / +16 % |
| FA20D | +14 / +10 % |
| BP 1.8 | +7 / +13 % |
| 2JZ-GT | +7 / +8 % |
| SR20DE | +7 / +4 % |
| VQ35 | +6 / +3 % |
| 4A-GE | +4 / +4 % |
| 7M-GE | +2 / +6 % |
| 4G63 | 0 / +10 % |

The tanh over the same spans rises +4 to +7 %. The median head rises +7 % (intake) and +9 % (exhaust). From 0.2 in
up, the model over the sheet on all ten:

- tanh-min: 0.94 to 1.19, rms 5.7 % (intake) and 5.1 % (exhaust);
- series: 0.86 to 1.01, always under, rms 6.7 % and 6.1 %.

**Reading.** The heads do not agree on a shape, and the tanh sits inside their scatter. The +15 % of the first three
sheets is the top of a spread, not the law. The K20A2 is high on both its sheets (+14 %, +15 %): that is the head's
own port. **No change to the port limit.** The two-stage build's stage B under-read (+5.6 % against its head's
+12.6 %) is a known per-head under-read, not a model shape.

**Low lift.** At 0.100 in (2.5 mm), the effective area over the curtain on the head's diameter:

- four-valve intake: 0.60 to 0.79, median 0.66;
- four-valve exhaust: 0.60 to 0.76, median 0.63;
- two-valve: LS7 0.70 / 0.67, 882 0.78 / 0.84.

The model's 0.73 governs at 0.100 in on four-valve cards with q above about 1.1. So it sits about 10 to 15 % over
the median production four-valve head where the overlap's flow begins. Below 2.5 mm nothing is measured.

## The overlay: twelve heads normalised (the lead's item 1, 02:22)

Each head's effective area over its valves' area is plotted against L / d and scaled by its own value at L / d =
0.25. All sheets pass d / 4, and their tops fall at different L / d. The heads are ten four-valve (above) and two
two-valve (the LS7, the 882). The arithmetic is **measured**.

Median (spread) against the tanh's own ratio to its value at d / 4:

| L / d | intake median | intake spread | exhaust median | exhaust spread | tanh |
|---|---|---|---|---|---|
| 0.100 | 0.52 | −7 to +15 % | 0.52 | −18 to +14 % | 0.63 |
| 0.150 | 0.74 | −4 to +12 % | 0.78 | −16 to +6 % | 0.82 |
| 0.200 | 0.91 | −5 to +7 % | 0.92 | −7 to +5 % | 0.94 |
| 0.300 | 1.04 | −4 to +5 % | 1.04 | −3 to +3 % | 1.03 |
| 0.350 | 1.05 | −6 to +7 % | 1.07 | −5 to +5 % | 1.05 |
| 0.400 | — | — | 1.07 | −5 to +6 % | 1.06 |

The four-valve medians equal the all-heads medians at every point, so no split by valve count shows.

**Reading.**

- Past the knee the heads collapse within ±3 to 7 %, onto the tanh to within 1 to 2 %. The port limit's form is the
  median head's, and it stays.
- The K20A2 sits at the top of the spread: 1.13 at L / d 0.35 against a median of 1.05. Between the two-stage build's
  lifts (L / d 0.243 to 0.361) its sheet gains about +14 %, where the median head and the model gain about +6 %. The
  build's stage B under-read is that head's own port.
- A card cannot carry a shape today: q sets level only. A per-card rise is the lead's ruling.
- Below the knee the sheets sit under the tanh, but there the model's 0.73 of the curtain governs (the low-lift
  coefficient above).

## The low-lift coefficient by the head's valves (applied; the lead's ruling on the table, 02:32)

`sim/valvetrain/area.fm`, valve.model 1:

- **Four valves:** 0.66 of π d L intake and 0.63 exhaust. These are the medians at 0.100 in of the nine production
  heads above, basis a secondary compilation at 28 in.
- **Two valves:** keep the base form's 0.73. The two heads measured, the LS7 at 0.70 / 0.67 and the 882 at 0.78 / 0.84,
  straddle it and are too few to move it. These heads run the old expression bit for bit.
- **Three and five valves:** no sheet, unchanged.
- **Below 0.100 in:** the same slope carried down, unmeasured.

**What it is not.** It does not close N3-16: that closes on the cards and the idle rows. It is tuned to no row.

Measured against the tip in scratch trees: full load in N m at 2500 / 4000 / 5500 rpm, tip → new.

| engine | 2500 | 4000 | 5500 |
|---|---|---|---|
| economy four | 133.3 → 135.5 | 135.0 → 135.3 | 147.1 → 145.5 |
| hot hatch, 4A-GE card | 136.3 → 137.6 | 137.7 → 137.8 | 145.3 → 144.9 |
| V10 | 410.8 → 422.9 | 437.5 → 444.4 | 439.6 → 454.7 |
| V12 | 579.7 → 581.5 | 561.1 → 560.5 | 628.3 → 654.1 |
| 3.2 | 282.1 → 286.8 | 297.3 → 297.6 | 276.9 → 280.7 |
| 7.0 (two-valve control) | identical | identical | identical |

The hot hatch's 4A-GE card was generated with TEE_CAM_RULE=1.

Warm idle, tip → new: speed, sd, manifold, burned share at intake closing.

| engine | speed | sd | manifold | share |
|---|---|---|---|---|
| economy four | 752, unchanged | 6.1, unchanged | 32.0, unchanged | 0.168, unchanged |
| hot hatch | 855, unchanged | 5.7 → 5.6 | 37.1 → 35.1 kPa | 0.289 → 0.276 |
| V10 | 899, unchanged | 1.6 → 1.7 | 43.4 → 41.0 kPa | 0.399 → 0.380 |
| V12 | 899, unchanged | 2.3 → 1.8 | 57.6 → 54.2 kPa | 0.488 → 0.474 |
| 3.2 | 848, unchanged | 3.7 → 3.6 | 51.0 → 50.1 kPa | 0.428 → 0.411 |
| 7.0 | identical | identical | identical | identical |

The four-valve idles moved toward their rows by 1 to 3 kPa, which is what happened, not a target.

The two-stage build's crossing (each stage held, 3000 to 8500 every 250) moved from 8012 to 7298 rpm, against the
real 5500 to 5900. It stays a recorded miss.

## A single dyno point's sensitivity floor (measured)

A first trial wrote the two-valve coefficient as 0.72696 against the old expression's 0.7269616, a difference of 2
parts per million. The 7.0's full load at 2500 rpm (tee-dyno --predict, default tolerance 0.01) moved from 593.5 to
591.1 N m, −0.4 %, repeatably in both directions.

With a tolerance of 0.001 and 40 to 60 cycles, both trees read 591.1 and 591.2 (neither met 0.001 by 60 cycles). So the gap is where the predictor's
stop rule ends its cycle average, not the idle or knock loops settling differently. The default reading of the tip is
itself 0.4 % from its own long mean.

So a single predicted point carries a floor of about ±0.5 %. A comparison or test bound finer than that on one point
reads the averaging, not the change. The predictor's users, the cam bake's "which position is best" and the near-bound
cases, should not decide on differences inside it (told to C).

## Tests

Two integrated valvetrain cases that the trial flipped are kept restated on their properties, and they pass on the
tip:

- case 8: the gain at the top is under half the gain low down;
- case 9: no dip at the change.

The next honest change to the lift-area curve would flip their old forms the same way.
