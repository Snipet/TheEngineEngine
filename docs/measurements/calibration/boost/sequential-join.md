# A sequential pair's join (the lead's ruling, 25 Nov)

**The fault.** The pair joined on the engine's own air (`drawMean` against `spec.boost.seqFlow`, closing under 0.8 of it):
the quantity the join collapses decided the join, and the 13B-kind rotary (`archetypes/rotary-twin-13`) hunted at
4250 rpm, full load, 26 to 106 kPa and 168 to 263 N m in 0.1 s windows, on a18f1bd4 and on the corrected map alike.
Real pairs change over on speed and load with a pre-spool: their signature is a dip of a couple of psi, not a hunt.

**The mechanism** (machine.fm; only on a build that states `spec.boost.seqRpm`, hidden; a build that states none keeps
the join on the flow to the bit): from 0.85 of the join speed the exhaust's transfer valve opens, at most 0.3, the second
compressor blowing off through its bleed; the charge valve opens above the join speed once the second compressor's
shut-off ratio at its shaft's speed reaches the charge pipe's, or after 1 s there; the transfer then fully open; the
pair leaves below 0.9 of the join speed or on a lift, not within 1 s of joining. 0.85, 0.9, 0.3 and 1 s: judgement.

**Tried and refuted**:
1. The join speed derived from the join flow at the design point (0.4 of the design speed): 2720 rpm on the rotary
   against its source's 4500, 1.65 out on the one build with a pair; it joined from 2720 and stood half-fed, 22 to 32 kPa
   from 2500 to 6000. No default is made from one engine: the speed is stated or the old join holds.
2. The transfer fully open through the pre-spool, the charge valve only on the ratio: the first turbine robbed, the
   rotary at 30 to 33 kPa at 4500, the half-fed second compressor never reaching the pipe's ratio (1.47 against 1.52).
3. The source's own constants (P's sources §16: pre-spool from about 3000 rpm, a forum's quote of the 1993 workshop
   manual; release below about 3300, Kelleher's measurement), 0.67 and 0.73 of the join: rising, the earlier pre-spool
   cost 1 % at 3250 to 4000 rpm and gained nothing; falling, the later release held the joined pair down to 3500 rpm,
   10 to 27 kPa against the single's 67 to 71 (this model's joined pair is weaker than its single below some 5000).

**Measured** (the rotary, `--set boost.seqRpm=4500`, full load, 250 rpm steps, 4 s each, the last 2 s in 0.1 s windows;
kPa range / mean N m):

| rpm | the flow join (today) | the stated join, rising | the stated join, falling |
|---|---|---|---|
| 3750 | 66-67 / 211 | 69-70 / 211 | 71-71 / 211 |
| 4000 | 69-71 / 216 | 68-71 / 216 | 68-71 / 216 |
| 4250 | **26-77 / 188** | 59-75 / 219 | 29-47 / 178 |
| 4500 | 44-47 / 199 | 64-68 / 225 | 45-47 / 200 |
| 4750 | 47-51 / 220 | 47-51 / 220 | 49-52 / 222 |
| 5000 and up | | the same within 1 kPa | |

The test (GateTest, "a sequential pair with a stated join speed changes over without a hunt", 3750 to 5250 rpm): every
0.1 s window within 15 % of its speed's mean; 4250 reads 59.4 to 76.6 against 67.3, inside but near the bound. Falling,
the pair stays joined to 4050 (0.9), so 4250 to 4500 read the joined pair's weaker 29 to 47: the speed hysteresis a
driver would feel as less boost coming down than going up there. **Open**: the joined pair is weaker than the single
below some 5000 rpm (the second compressor's old item); the rotary states its 4500 when P commits it.
