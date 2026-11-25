# The rotary's joined pair: predictions, 25 Nov ~19:46, before any run
A working sequential pair, joined (the 13B-REW: both turbos parallel once the charge and exhaust control valves are
open; P's sources section 16: the joined pair holds the target, some 0.7 bar, to the redline): each takes about half.
At 5000 and 6500 rpm, per turbo:
- compressor flow: the secondary within 20 % of the primary (about 0.09-0.11 kg/s each);
- pressure ratio: both at the target's, about 1.69 (0.70 bar);
- shafts: within 10 % of each other;
- turbine inlets: within 10 % of each other.
The model today (N1, measured): the secondary a tenth of the flow, its shaft at 10.7 k against the primary's 13-16 k,
inlets 1.6-1.8 against 2.1-3.1 bar, boost 0.45-0.53 bar.
The suspected cause: the secondary is fed only through the transfer orifice (aTransfer = its own nozzle's area) in
series with its own nozzle. Predicted: with aTransfer at 10x (no restriction) the split comes within 20 %, the boost
reaches 0.70 bar with the gate working, and torque at 5000 reaches the cited ~294 N m band (4500-5500); at 3x within
30 %.

## Measured (the tip after the diesel package, e9b2ef3f; scratch copies with the transfer's area scaled; the last 5 of 10 s)

| transfer area | rpm | N m | boost kPa | compressor flow pri / sec kg/s | shaft pri / sec rad/s | p3 pri / sec bar | flags |
|---|---|---|---|---|---|---|---|
| 1x (today) | 5000 | 229.1 | 49.4 | 0.107 / 0.017 | 9974 / 9659 | 1.94 / 1.50 | 0.59 |
| 3x | 5000 | 232.0 | 45.4 | 0.075 / 0.049 | 9317 / 9219 | 1.76 / 1.65 | 0 |
| 10x | 5000 | 232.9 | 45.2 | 0.069 / 0.054 | 9265 / 9229 | 1.73 / 1.68 | 0 |
| 1x (today) | 6500 | 227.8 | 54.4 | 0.158 / 0.012 | 12164 / 10676 | 2.43 / 1.60 | 1.68 |
| 3x | 6500 | 231.4 | 46.0 | 0.103 / 0.064 | 10105 / 9805 | 2.02 / 1.85 | 0 |
| 10x | 6500 | 232.8 | 45.5 | 0.095 / 0.073 | 9964 / 9838 | 1.97 / 1.88 | 0 |

The gate is shut in every row. The cited figure is about 294 N m at 5000 (band 4500-5500).

**There are two faults.**

1. **The secondary is starved by its feed** (N1's suspicion, confirmed). machine.fm feeds the second housing only
   through the transfer orifice from housing 0, with area aTransfer = the second turbine's own nozzle. The two equal
   areas in series pass about 0.71 of one. So the secondary gets a tenth of the flow and the primary chokes.
   Unrestricting the transfer (3x-10x) evens the pair: the secondary carries 65-78 % of the primary, the shafts are
   within 3 %, and the choke flags go.
2. **The evened pair is still weak.** It makes 45 kPa with the gate shut, and the torque gains under 2 % (229 -> 233
   N m at 5000, still -21 % against the cited 294). The pair's turbines are too large for the flow: p3 is 1.7-2.0 bar.
   That is the pair's match (5c85736b: 0.5 + 0.5 of the design flow at 0.85 of the redline), task 17's "turbines
   oversized at the redline match".

**The predictions:** the even split was right (at 10x within 22 %). The boost at 0.70 bar and the torque in the cited
band were wrong.

**For the next window:**
- the transfer as an unrestricted path (the joined 13B-REW's both turbines see the manifold; no new number: the
  housing's own inlet);
- with it, the sequential pair's match re-derived (the design point for a pair, a topology question).

Landing fault 1 alone would trade a choked primary for a balanced pair that makes no more torque, and it changes the
rotary's sound and flags, so it is not landed alone.
