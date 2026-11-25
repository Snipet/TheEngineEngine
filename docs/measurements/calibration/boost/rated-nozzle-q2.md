# Research Q2: the open nozzle matched on the turbine's own efficiency at rated. Predictions, 25 Nov ~15:55, before any run
The law (turbine.fm): eta = etaMax x (2 - x) vk, x = BSR / 0.7, vk = 1 with the vanes open. The power balance at
rated: m_c cp T1 tau / eta_c = m_t cpE T3 eta_t(BSR) (1 - (p4/p3)^(1/4)). It solves for p3; the open area passes the
rated flow at p3 in the match's choked form.
By hand at the tractor's rated point (tip 511 m/s, so the turbine tip about 460 m/s; T3 900 K; p3 about 1.2 p2):
cs about 750 m/s, BSR about 0.61, x 0.87, x(2 - x) 0.98.
1. The engine's own BSR at rated, open vanes: 0.55-0.65, so the turbine's efficiency is at least 0.95 of its peak.
2. So matching on it gives nearly the peak-efficiency balance (n21's variant): p3/p2 at the match 0.78-0.84, and the
   open area 1.3-1.5 times today's. That is too large, as n21 found: the turbine at that area cannot hold 202 at rated.
3. The gap between the balance and the engine is in what the balance leaves out, measured at rated steady:
   - p4, the turbine's outlet, over the ambient (the pipes after it): 105-115 kPa;
   - T3 under 900 K at the wheel (the housing's heat loss): 870-900;
   - the compressor's efficiency at its point, 0.69, against its peak;
   - the shaft's bearing loss, a few % of the turbine's power;
   - pulsation: the cycle mean of the turbine's power under the mean-state value (unmeasured; suspected).
   Prediction: p4 and the compressor's efficiency carry most of it; the bearing loss is small.

## Measured (the package tree plus scratch instrumentation; rated, the last 30 of 60 s, 1 s means)

| build | rpm | BSR | x(2-x) | etaT | etaC | p3 kPa | p4 kPa | p3/p2 | T3 K | Pt kW | Pc kW | friction kW | vanes |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| tractor (202, 72.4 mm) | 1900 | 0.678 | 0.999 | 0.717 | 0.715 | 361 | 123.3 | 1.17 | 835 | 110.1 | 109.6 | 0.46 | 1.00 |
| truck's sets (260) | 2100 | 0.681 | 0.999 | 0.692 | 0.697 | 430 | 116.7 | 1.40 | 803 | 115.0 | 114.5 | 0.53 | 0.78 |
| 2.0 | 4000 | 0.647 | 0.994 | 0.715 | 0.740 | 324 | 137.2 | 1.21 | 911 | 23.6 | 23.2 | 0.37 | 1.00 |
| pickup | 2800 | 0.638 | 0.992 | 0.709 | 0.741 | 362 | 131.9 | 1.20 | 846 | 67.5 | 67.1 | 0.39 | 0.95 |

**The open area under each matching assumption,** as a multiple of the area the engine uses at rated:
A_eng = mNoz sqrt(R T3) / (0.667 p3), at its measured state. Each balance is solved for p3 on the measured compressor
power and nozzle flow, and the area is the match's choked form at that p3.

| build | A_eng cm2 | today (1.247 p2, 900 K) | balance, peak eta, p4 101, 900 K | balance, the law's eta at the rated BSR, p4 101, 900 K | the same, measured p4 | the same, measured p4 and T3 |
|---|---|---|---|---|---|---|
| tractor | 13.78 | 0.98 | 1.39 | 1.39 (BSR 0.68) | 1.14 | 1.01 |
| truck | 10.83 | 1.18 | 1.52 | 1.52 (0.70) | 1.32 | 1.07 |
| 2.0 | 3.88 | 0.96 | 1.37 | 1.36 (0.65) | 1.01 | 1.02 |
| pickup | 9.00 | 0.99 | 1.48 | 1.47 (0.64) | 1.13 | 1.02 |

**What it shows:**
1. **The turbine's own efficiency at rated is its peak.** At rated the BSR is 0.64-0.70 on every diesel, where the
   law's x(2 - x) is 0.99-1.00. Matching on it gives the peak-efficiency balance's area within 0.7 %, at 1.36-1.52 times
   the area the engine uses: too large, as n21 found.
2. **The missing term is p4,** the turbine's outlet, which is 117-137 kPa and not 101. It is the pipes after the
   turbine, card G's exhaust. With it, the balance's area falls to 1.01-1.32. With T3 as measured too (803-911 against
   900), it falls to 1.01-1.07, which closes the balance. So the mean-state balance holds. The cycle's pulsation is not
   a large term on three builds; the truck's 7 % sits with vanes at 0.78, where the vanes' factor (0.85 + 0.15 v) and
   the flow through partly shut vanes enter.
3. **Today's 1.247 p2 lands within 4 % on three of four by accident:** it stands in for p4 and T3 together. The truck is
   the exception (1.18): it is choke-limited at 2100, with the back-pressure limit holding the vanes.
4. **The bearing's loss is 0.4 % of the turbine's power,** and the turbine and the compressor balance within 0.5 %.

**The predictions:**
- 1 (BSR 0.55-0.65): right in substance; the measured 0.64-0.68 sits at the range's top and over it.
- 2 (the area 1.3-1.5 times): right. The p3/p2 of 0.78-0.84 was low: the measured is 0.84-0.97.
- 3: p4 was predicted 105-115 and measured 117-137, under. T3 was predicted 870-900 and measured 803-911, wrong for
  the tractor and the truck. The bearing's loss being small: right.
- The compressor's efficiency at its point was not a term here, since the balance used the measured compressor power.
  Its share is open.
- Pulsation: suspected large, measured small (within 2 % on three).

**For the rule:** an area matched on the turbine's efficiency gives every build both ends only if the match knows p4 at
rated, from the build's exhaust after the turbine. The machine has no such number at sizing; the exhaust is G's, and
its back pressure at the rated flow would have to be asked of G. I have built nothing.
