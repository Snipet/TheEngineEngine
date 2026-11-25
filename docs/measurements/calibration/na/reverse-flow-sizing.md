# Sizing route (b) of N3-16: what a low-lift reverse-flow coefficient would have to be

Card N1, 25 Nov, 01:35 UTC (by `date -u`). The lead's 01:33 request. This is a diagnostic: it sizes a cause and does not
fit a part. Nothing here goes into a build or the model. The knob existed only in scratch trees.

## The question

The warm idle's burned share at intake closing reads 0.40 to 0.49 on the big-cam engines, after 68e683f1. The measured
band is about 0.20 to 0.27 (N3-16: the thesis's points; Cavina's orifice form gives 0.17 to 0.21). Two routes are open:

- (b) the valve's discharge coefficient in reverse flow at low lift, whose sources we cannot read;
- the drive, D's: the pressure across the valves through the overlap.

How large a reverse-flow reduction would (b) need to carry the gap alone?

## The probe-only knob

A scratch copy of `sim/engine.fm` scales a valve's effective area by a factor `s` when the previous sample's flow
through it ran the wrong way and its lift is under 2 mm. The wrong way is the exhaust drawing back in (`mE < 0`) or the
intake pushing out (`mI > 0`). Under 2 mm is the band where the overlap's flow runs (D's trace: 0.3 to 2 mm). The
direction is the previous sample's, one sample of lag, which is acceptable for a sizing.

Tip at af2debe5, with 68e683f1 in. Shares from `[.probe-idle]` (p-idle-warm's conditions, the last three seconds' mean).
Full load from `tee-dyno --predict`, single points. All **measured**.

## The idle shares

| s | V10 | V12 | hot hatch (buckets) | economy four |
|---|---|---|---|---|
| 1.0 (today) | 0.399 | 0.488 | 0.463 | 0.168 |
| 0.5 | 0.306 | 0.383 | 0.341 | 0.164 |
| 0.3 | 0.267 | 0.314 | 0.271 | 0.160 |
| 0.2 | 0.244 | 0.268 | 0.239 | 0.158 |
| 0.1 | 0.217 | 0.226 | 0.202 | 0.155 |

The three big-cam engines enter the band at s = 0.2 to 0.3. The economy four, already under it, moves only 0.168 to
0.158, so the mild engine does not break. Every idle held its speed (898 to 899 rpm; 851 to 856; 751 to 752).

## Full load at the needed value (s = 0.2 against 1.0, N m)

| engine | speed | s = 1.0 | s = 0.2 | change |
|---|---|---|---|---|
| V10 | 3000 rpm | 447.1 | 473.5 | +6 % |
| V10 | 5300 rpm | 431.9 | 488.1 | +13 % |
| V12 | 2100 rpm | 518.9 | 583.8 | +13 % |
| V12 | 5400 rpm | 618.4 | 655.4 | +6 % |
| economy four | 2500 rpm | 133.2 | 142.3 | +7 % |
| economy four | 5000 rpm | 147.5 | 150.3 | +2 % |

## Reading it

Route (b) carries the gap only if a valve passes reverse flow at low lift with 0.2 to 0.3 of its forward effective area.
That is a reverse discharge coefficient on the curtain of some 0.15 to 0.22, against today's 0.66 to 0.73 forward.

What I would believe is **judgement from memory, not a source**: a poppet valve's reverse coefficient below its forward
one, perhaps 0.6 to 1.0 of it at low lift. In reverse, the jet separates off the valve head's back face rather than the
seat's chamfer, and steady-flow rigs that measure both directions show the reverse lower but of the same order. A
reverse coefficient of 0.2 to 0.3 of the forward one is beyond that. It would make the valve a strong diode at low lift,
which nothing I know of measures.

So (b) can plausibly carry a part of the gap, perhaps the step from s = 1.0 to 0.6 to 0.8. That is 0.03 to 0.06 of the
share on the V10 and V12, judged by interpolating the table. It cannot carry the whole. The remainder, most of it, lies
in the drive: the pressure across the valves through the overlap, which is D's. The economy four's insensitivity says
the effect is the overlap's alone, as expected.

If a measured reverse coefficient against lift over diameter is found (Heywood's figure, Algieri et al. 2011, Blair),
this table gives at once what it is worth.
