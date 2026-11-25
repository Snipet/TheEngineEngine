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

## Tests

Two integrated valvetrain cases that the trial flipped are kept restated on their properties, and they pass on the
tip:

- case 8: the gain at the top is under half the gain low down;
- case 9: no dip at the change.

The next honest change to the lift-area curve would flip their old forms the same way.
